#include "net/server.h"

#include <vector>

#include "logger.h"
#include "adb.h"
#include "net/serializer.h"

Server::Server(int port, const ConnectionListener& connectionListener) :
	port(port),
	connectionListener(connectionListener),
	acceptor(context)
{
	try {
		tcp::endpoint endpoint(tcp::v4(), port);
		acceptor.open(endpoint.protocol());

		acceptor.set_option(tcp::acceptor::reuse_address(true));

		acceptor.bind(endpoint);
		acceptor.listen();

		logger << "[SERVER] Initialized on port " << port << std::endl;
		
		adb::reverse(port);
		adb::forward(8554);
	}
	catch(std::exception& e)
	{
		logger << "[SERVER] CRITICAL INIT ERROR: " << e.what() << std::endl;
		throw;
	}
}

Server::HostInfo Server::GetHostInfo()
{
	std::string name = asio::ip::host_name();
	std::string ip_str = "127.0.0.1"; // Default fallback

	try {
		// Get the active network adapter by creating a dummy UDP connection 
		// and let the OS determine what network adapter will be used
		udp::resolver resolver(context);
		udp::socket socket(context);

		// This might throw if there is NO network adapter enabled
		socket.connect(udp::endpoint(asio::ip::make_address("8.8.8.8"), 80));
		ip_str = socket.local_endpoint().address().to_string();
		socket.close();
	}
	catch (std::exception& e) 
	{
		logger << "[SERVER] Warning: Could not detect local IP (" << e.what() << "). Defaulting to localhost.\n";
	}

	return { name, ip_str, std::to_string(port) };
}

void Server::Send(int id, const unsigned char* bytes, size_t size) const
{
	std::shared_ptr<Connection> connection;
	{
		std::lock_guard<std::mutex> lock(connectionsMutex);
		auto it = connections.find(id);
		if (it == connections.end())
			return;
		connection = it->second;
	}

	// Send outside the lock so a stalled socket never blocks accept/erase.
	if (connection)
	{
		try
		{
			connection->Send(bytes, size);
		}
		catch (const std::exception& e)
		{
			logger << "[SERVER] Send failed: " << e.what() << "\n";
		}
	}
}

void Server::Start()
{
	if (!acceptor.is_open()) 
	{
		logger << "[SERVER] Cannot start: Acceptor is not open.\n";
		return;
	}

	try
	{
		TCPDoAccept();

		thread = std::thread([this]() {
			while (true)
			{
				try {
					context.run();
					break;
				}
				catch (std::exception& e) {
					logger << "[SERVER] CRITICAL EXCEPTION in IO Thread: " << e.what() << std::endl;
				}
				catch (...) {
					logger << "[SERVER] Unknown exception in IO Thread.\n";
				}
			}
		});
		
		logger << "[SERVER] Started" << std::endl;
	}
	catch (std::exception e)
	{
		logger << "[SERVER] Start failed: " << e.what() << "\n";
	}
}

void Server::Close()
{
	logger << "[SERVER] Closing...\n";

	asio::error_code ec;
	acceptor.close(ec);
	if (ec) logger << "[SERVER] Error closing acceptor: " << ec.message() << "\n";

	// Snapshot the list so closing a socket (which can complete on the I/O
	// thread) never races with the erase in OnConnectionDisconnected.
	std::vector<std::shared_ptr<Connection>> connectionsCopy;
	{
		std::lock_guard<std::mutex> lock(connectionsMutex);
		for (const auto& [id, conn] : connections)
			connectionsCopy.push_back(conn);
	}
	for (std::shared_ptr<Connection>& conn : connectionsCopy)
	{
		conn->Close(true);
	}

	context.stop();

	if (thread.joinable())
	{
		thread.join();
	}

	logger << "[SERVER] Closed.\n";

	adb::kill(port);
}

void Server::TCPDoAccept()
{
	acceptor.async_accept([&, this](asio::error_code ec, tcp::socket socket) {
		if (!ec)
		{
			logger << "[SERVER] Device connected." << socket.remote_endpoint() << std::endl;

			// Never let a half-open connection block the I/O thread forever.
			// The descriptor is small; five seconds is plenty.
			{
				DWORD timeoutMs = 5000;
				setsockopt(socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO,
					reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
			}

			// Read the only message sent by the client (the device descriptor).
			// The descriptor is variable-length, so read its fields exactly instead
			// of assuming that one read_some() returns the whole message. A single
			// read could also truncate descriptors larger than 512 bytes.
			try
			{
				std::vector<uint8_t> buffer;

				auto appendUInt16 = [&buffer](uint16_t value) {
					buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
					buffer.push_back(static_cast<uint8_t>(value & 0xFF));
				};
				auto readExact = [&socket](void* data, size_t size) {
					asio::read(socket, asio::buffer(data, size));
				};
				auto readUInt16 = [&socket]() {
					uint8_t bytes[2];
					asio::read(socket, asio::buffer(bytes, sizeof(bytes)));
					return static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
				};
				auto readString = [&]() {
					uint16_t length = readUInt16();
					appendUInt16(length);
					size_t offset = buffer.size();
					buffer.resize(offset + length);
					if (length > 0) {
						readExact(buffer.data() + offset, length);
					}
				};
				auto readResolutions = [&]() {
					uint16_t count = readUInt16();
					appendUInt16(count);
					size_t offset = buffer.size();
					size_t bytes = static_cast<size_t>(count) * 4;
					buffer.resize(offset + bytes);
					if (bytes > 0) {
						readExact(buffer.data() + offset, bytes);
					}
				};

				readString();       // device name
				readString();       // RTSP URL
				readResolutions();  // front camera resolutions
				readResolutions();  // back camera resolutions

				uint16_t filterCount = readUInt16();
				appendUInt16(filterCount);
				for (uint16_t i = 0; i < filterCount; ++i) {
					readString();   // filter name
					uint8_t category = 0;
					readExact(&category, 1);
					buffer.push_back(category);
				}

				auto descriptor = Serializer::DeserializeDeviceDescriptor(buffer.data(), buffer.size());

				auto conn = std::make_shared<Connection>(
					std::move(socket),
					descriptor,
					std::bind(&Server::OnConnectionDisconnected, this, std::placeholders::_1),
					std::bind(&Server::OnConnectionReportingError, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
				);
				{
					std::lock_guard<std::mutex> lock(connectionsMutex);
					const int id = nextConnectionId++;
					conn->SetId(id);
					descriptor.SetConnectionId(id);
					connections[id] = conn;
				}
				conn->Start();

				connectionListener.OnDeviceConnected(descriptor);
			}
			catch (const std::exception& e)
			{
				logger << "[SERVER] Failed to read device descriptor: " << e.what() << std::endl;
			}
		}
		else if (ec != asio::error::operation_aborted)
		{
			logger << "[SERVER] Accept Error: " << ec.message() << std::endl;
		}

		if (acceptor.is_open()) {
			TCPDoAccept();
		}
	});
}

void Server::OnConnectionDisconnected(std::shared_ptr<Connection> connection)
{
	logger << "[SERVER] Device disconnected: " << connection->descriptor.name() << std::endl;

	{
		std::lock_guard<std::mutex> lock(connectionsMutex);
		connections.erase(connection->Id());
	}
	connectionListener.OnDeviceDisconnected(connection->descriptor);
}

void Server::OnConnectionReportingError(std::shared_ptr<Connection> connection, const uint8_t* bytes, size_t size)
{
	auto report = Serializer::DeserializeErrorReport(bytes, size);
	connectionListener.OnDeviceErrorReported(connection->descriptor, report);
}
