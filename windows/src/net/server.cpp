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
	acceptor.async_accept([this](asio::error_code ec, tcp::socket socket) {
		if (!ec)
		{
			logger << "[SERVER] Device connected." << socket.remote_endpoint() << std::endl;

			// The descriptor is read asynchronously. A blocking read here would
			// stall the whole I/O thread whenever a phone connected without
			// sending anything, which killed the working connection.
			auto sock = std::make_shared<tcp::socket>(std::move(socket));
			sock->set_option(tcp::no_delay(true));
			ReadDescriptor(sock, std::vector<uint8_t>());
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

void Server::ReadDescriptor(std::shared_ptr<tcp::socket> socket, std::vector<uint8_t> buffer)
{
	auto chunk = std::make_shared<std::vector<char>>(4096);

	socket->async_read_some(asio::buffer(*chunk),
		[this, socket, buffer, chunk](asio::error_code ec, size_t bytes) mutable {
			if (ec)
			{
				logger << "[SERVER] Failed to read device descriptor: " << ec.message() << std::endl;
				return;
			}

			for (size_t i = 0; i < bytes; ++i)
				buffer.push_back(static_cast<uint8_t>((*chunk)[i]));

			std::string deviceName;
			std::string deviceUrl;
			std::vector<DeviceDescriptor::Resolution> frontResolutions;
			std::vector<DeviceDescriptor::Resolution> backResolutions;
			Video::Filter::Registry filters;
			if (!TryParseDescriptor(buffer, deviceName, deviceUrl, frontResolutions, backResolutions, filters))
			{
				if (buffer.size() > 64 * 1024)
				{
					logger << "[SERVER] Device descriptor is too large\n";
					return;
				}
				ReadDescriptor(socket, buffer);
				return;
			}

			if (deviceName.empty() || deviceUrl.rfind("rtsp://", 0) != 0)
			{
				logger << "[SERVER] Ignoring malformed device descriptor (name=" << deviceName
					<< ", url=" << deviceUrl << ")\n";
				return;
			}

			DeviceDescriptor descriptor(deviceName, deviceUrl, "tcp",
				frontResolutions, backResolutions, filters);

			auto conn = std::make_shared<Connection>(
				std::move(*socket),
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
		});
}

// Parses the device descriptor out of the received bytes. Returns false when
// more data is needed.
bool Server::TryParseDescriptor(const std::vector<uint8_t>& data,
	std::string& name, std::string& url,
	std::vector<DeviceDescriptor::Resolution>& frontResolutions,
	std::vector<DeviceDescriptor::Resolution>& backResolutions,
	Video::Filter::Registry& filters)
{
	size_t offset = 0;

	auto has = [&](size_t count) { return offset + count <= data.size(); };
	auto readU16 = [&](uint16_t& value) -> bool {
		if (!has(2)) return false;
		value = static_cast<uint16_t>((data[offset] << 8) | data[offset + 1]);
		offset += 2;
		return true;
	};
	auto readString = [&](std::string& value) -> bool {
		uint16_t length = 0;
		if (!readU16(length)) return false;
		if (!has(length)) return false;
		value.assign(reinterpret_cast<const char*>(data.data() + offset), length);
		offset += length;
		return true;
	};
	auto readResolutions = [&](std::vector<DeviceDescriptor::Resolution>& list) -> bool {
		uint16_t count = 0;
		if (!readU16(count)) return false;
		if (!has(static_cast<size_t>(count) * 4)) return false;
		for (uint16_t i = 0; i < count; ++i)
		{
			const uint16_t width = static_cast<uint16_t>((data[offset] << 8) | data[offset + 1]);
			const uint16_t height = static_cast<uint16_t>((data[offset + 2] << 8) | data[offset + 3]);
			offset += 4;
			list.push_back(DeviceDescriptor::Resolution(width, height));
		}
		return true;
	};

	if (!readString(name) || !readString(url))
		return false;

	if (!readResolutions(frontResolutions) || !readResolutions(backResolutions))
		return false;

	uint16_t filterCount = 0;
	if (!readU16(filterCount))
		return false;

	for (uint16_t i = 0; i < filterCount; ++i)
	{
		std::string filterName;
		if (!readString(filterName))
			return false;
		if (!has(1))
			return false;
		const auto category = static_cast<Video::Filter::Category>(data[offset++]);
		filters[category].push_back(filterName);
	}

	return true;
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
