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
			try
			{
				onBytesReceived(this->shared_from_this(), (const uint8_t*)byteBuffer, bytes);
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