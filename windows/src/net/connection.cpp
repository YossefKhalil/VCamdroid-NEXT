#include "net/connection.h"

#include "logger.h"

Connection::Connection(tcp::socket socket, DeviceDescriptor& descriptor, OnDisconnectedListener onDisconnectedListener, OnBytesReceived onBytesReceived) 
	: socket(std::move(socket)),
	descriptor(descriptor),
	onDisconnectedListener(onDisconnectedListener),
	onBytesReceived(onBytesReceived)
{
	active = false;
}

void Connection::Start()
{
	Read();
}

void Connection::Read()
{
	// Keep the connection alive for as long as the async read is outstanding.
	// Without this, Server::OnConnectionDisconnected can destroy the object
	// while this handler is still running.
	auto self = shared_from_this();

	socket.async_read_some(asio::buffer(byteBuffer, sizeof(byteBuffer)), [this, self](asio::error_code ec, size_t bytes) {
		if (!ec)
		{
			incoming.insert(incoming.end(), byteBuffer, byteBuffer + bytes);
			try
			{
				ProcessIncoming();
			}
			catch (const std::exception& e)
			{
				logger << "[CONNECTION] onBytesReceived failed: " << e.what() << std::endl;
			}
			if (!closed)
				Read();
		}
		else
		{
			// When the connection is closed by the server this function may still
			// be called and then the socket will be invalid so wrap the endpoint
			// lookup in a try/catch. Close() itself is always called so the
			// disconnect callback can never be skipped.
			try
			{
				logger << "[CONNECTION@" << this->socket.remote_endpoint() << "] Closed with error code: " << ec << std::endl;
			}
			catch (const std::exception& e)
			{
				logger << "[CONNECTION] Closed with error code: " << ec << " (" << e.what() << ")" << std::endl;
			}
			Close();
		}
	});
}

// The phone sends status / error reports over the control connection. They
// are small but can still be split across TCP reads, so accumulate until a
// complete report is available.
void Connection::ProcessIncoming()
{
	while (!incoming.empty())
	{
		size_t offset = 0;

		// severity byte
		if (incoming.size() < offset + 1)
			return;
		offset += 1;

		auto readString = [&](std::string& value) -> bool {
			if (incoming.size() < offset + 2)
				return false;
			const uint16_t length = static_cast<uint16_t>(
				(static_cast<uint16_t>(incoming[offset]) << 8) | incoming[offset + 1]);
			offset += 2;
			if (incoming.size() < offset + length)
				return false;
			value.assign(reinterpret_cast<const char*>(incoming.data() + offset), length);
			offset += length;
			return true;
		};

		std::string error;
		std::string description;
		if (!readString(error) || !readString(description))
			return;

		onBytesReceived(shared_from_this(), incoming.data(), offset);
		incoming.erase(incoming.begin(), incoming.begin() + offset);
	}
}

void Connection::Send(std::string message)
{
	Send(reinterpret_cast<const unsigned char*>(message.data()), message.size());
}

void Connection::Send(const unsigned char* bytes, size_t size)
{
	if (closed)
		return;

	// The socket can be torn down by the I/O thread at any time, so never let
	// a UI-thread command escape as an exception.
	asio::error_code ec;
	socket.send(asio::buffer(bytes, size), 0, ec);
	if (ec)
		logger << "[CONNECTION] Send failed: " << ec.message() << std::endl;
}

void Connection::Close(bool stoppedByServer)
{
	// Read errors and Server::Close can race; only the first caller reports the
	// disconnect (and only once).
	if (closed.exchange(true))
		return;

	if (stoppedByServer)
	{
		// Sometimes socket.cancel() makes the app to not respond
		// but sometimes is required to successfully remove the adb
		// port forwarding
		// TODO...
		//socket.cancel();
		asio::error_code ec;
		socket.close(ec);
	}
	else
	{
		asio::error_code ec;
		socket.close(ec);
		if (onDisconnectedListener)
			onDisconnectedListener(this->shared_from_this());
	}
}