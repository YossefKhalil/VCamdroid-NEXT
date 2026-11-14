#include "rtsp/manager.h"

#include "logger.h"
#include "net/serializer.h"

#include <algorithm>
#include <iterator>

namespace RTSP
{
	Manager::Manager(const Server& server, OnFrameReceivedCallback onFrameReceivedCallback, OnStatsReceivedCallback onStatsReceivedCallback) 
		: Receiver(onFrameReceivedCallback, onStatsReceivedCallback),
		server(server)
	{
		streamingDevice = -1;

		// If RTSP is not up yet (the phone is still creating StreamActivity),
		// repeat the activation packet so the stream eventually starts even
		// when the first packet arrives before the phone is listening.
		SetRetryCallback([this]()
		{
			if (streamingDevice >= 0 && streamingDevice < static_cast<int>(descriptors.size()))
			{
				auto serialized = Serializer::SerializeStreamOptions(lastOptions);
				serialized[0] = Command::ACTIVATION;
				SendToStreamingDevice( serialized.data(), serialized.size());
			}
		});
	}

	void Manager::SendToStreamingDevice(const unsigned char* bytes, size_t size)
	{
		if (streamingDevice < 0 || streamingDevice >= static_cast<int>(descriptors.size()))
			return;

		const int id = descriptors[streamingDevice].connectionId();
		if (id >= 0)
			server.Send(id, bytes, size);
	}

	void Manager::AddDescriptor(DeviceDescriptor& descriptor)
	{
		// A reconnect can produce a second descriptor for the same stream URL
		// before the old connection's disconnect is processed. Replace the old
		// entry instead of stacking duplicates.
		const std::string url = descriptor.url();
		auto existing = std::find_if(descriptors.begin(), descriptors.end(),
			[&url](const DeviceDescriptor& item) { return item.url() == url; });

		if (existing != descriptors.end())
		{
			const int index = static_cast<int>(std::distance(descriptors.begin(), existing));
			if (index == streamingDevice)
			{
				Disconnect();
			}
			else if (index < streamingDevice)
			{
				--streamingDevice;
			}
			descriptors.erase(existing);
		}

		logger << "[RTSP Manager] Device descriptor: " << descriptor.name()
			<< " url=" << descriptor.url()
			<< " front=" << descriptor.frontResolutions().size()
			<< " back=" << descriptor.backResolutions().size()
			<< " filters=" << descriptor.filters().size() << "\n";

		descriptors.push_back(std::move(descriptor));
	}

	void Manager::RemoveDescriptor(DeviceDescriptor& descriptor)
	{
		auto it = std::find(descriptors.begin(), descriptors.end(), descriptor);
		if (it == descriptors.end())
			return;

		const int removedIndex = static_cast<int>(std::distance(descriptors.begin(), it));

		if (removedIndex == streamingDevice)
		{
			// The active device went away: stop retrying its RTSP URL and clear
			// the streaming index so the next device can auto-start.
			Disconnect();
		}
		else if (removedIndex < streamingDevice)
		{
			// Indices are also the connection ids used by Server::Send, so they
			// shift together when a connection is removed.
			--streamingDevice;
		}

		descriptors.erase(it);
	}

	void Manager::Disconnect()
	{
		Stop();
		streamingDevice = -1;
	}

	void Manager::Connect2Stream(int descriptorId, const StreamOptions& options)
	{
		if (descriptorId < 0 || descriptorId >= static_cast<int>(descriptors.size()))
			return;

		this->streamingDevice = descriptorId;

		auto& descriptor = descriptors[descriptorId];
		auto& url = descriptor.url();

		logger << "[RTSP Manager] Connecting to stream " << url
			<< " (" << descriptor.protocol() << ")\n";

		lastOptions = options;
		
		Stop();
		
		auto serializedOptions = Serializer::SerializeStreamOptions(options);
		serializedOptions[0] = Command::ACTIVATION;

		SendToStreamingDevice( serializedOptions.data(), serializedOptions.size());

		Start(url, descriptor.protocol(), options.resolution.first, options.resolution.second);
	}

	void Manager::SetResolution(unsigned short width, unsigned short height)
	{
		const unsigned char bytes[5] = {
			Command::RESOLUTION,							// First byte is the packet type
			static_cast<unsigned char>(width & 0xFF),		// Low byte of width
			static_cast<unsigned char>(width >> 8),			// High byte of width
			static_cast<unsigned char>(height & 0xFF),		// Low byte of height
			static_cast<unsigned char>(height >> 8),		// High byte of height
		};

		SendToStreamingDevice( bytes, 5);
	}

	void Manager::SetFPS(int fps)
	{
		const unsigned char bytes[2] = { Command::FPS, (uint8_t)fps };
		SendToStreamingDevice( bytes, 2);
	}

	void Manager::SetBitrate(int bitrate)
	{
		const unsigned char bytes[3] = { 
			Command::BITRATE, 
			static_cast<unsigned char>(bitrate & 0xFF),		// Low byte of bitrate
			static_cast<unsigned char>(bitrate >> 8),         // High byte of bitrate
		};
		SendToStreamingDevice( bytes, 3);
	}

	void Manager::SetAdaptiveBitrate(int min, int max)
	{
		const unsigned char bytes[5] = {
			Command::ADAPTIVE_BITRATE,
			static_cast<unsigned char>(min & 0xFF),		// Low byte of bitrate
			static_cast<unsigned char>(min >> 8),         // High byte of bitrate
			static_cast<unsigned char>(max & 0xFF),		// Low byte of bitrate
			static_cast<unsigned char>(max >> 8),         // High byte of bitrate
		};
		SendToStreamingDevice( bytes, 5);
	}

	void Manager::SetStabilization(bool enabled)
	{
		const unsigned char bytes[2] = { Command::STABILIZATION, enabled ? 1 : 0 };
		SendToStreamingDevice( bytes, 2);
	}

	void Manager::SetFlash(bool enabled)
	{
		const unsigned char bytes[2] = { Command::FLASH, enabled ? 1 : 0 };
		SendToStreamingDevice( bytes, 2);
	}

	void Manager::SetFocusMode(int mode)
	{
		const unsigned char bytes[2] = { Command::FOCUS, (uint8_t)mode };
		SendToStreamingDevice( bytes, 2);
	}

	void Manager::SetH265Codec(bool enabled)
	{
		const unsigned char bytes[2] = { Command::CODEC, enabled ? 1 : 0 };
		SendToStreamingDevice( bytes, 2);
	}

	void Manager::SwapCamera()
	{
		const unsigned char bytes[1] = { Command::CAMERA };
		SendToStreamingDevice( bytes, 1);
	}

	void Manager::Rotate(uint8_t degrees)
	{
		const unsigned char bytes[2] = { Command::ROTATION, degrees };
		SendToStreamingDevice( bytes, 2);
	}

	void Manager::Zoom(float factor)
	{
		int bits;
		std::memcpy(&bits, &factor, sizeof(float));

		const unsigned char bytes[5] = {
			Command::ZOOM,
			static_cast<unsigned char>(bits & 0xFF),
			static_cast<unsigned char>((bits >> 8) & 0xFF),
			static_cast<unsigned char>((bits >> 16) & 0xFF),
			static_cast<unsigned char>((bits >> 24) & 0xFF),
		};

		SendToStreamingDevice( bytes, 5);
	}

	void Manager::FlipHorizontally()
	{
		const unsigned char bytes[2] = { Command::FLIP, 1 };
		SendToStreamingDevice( bytes, 2);
	}

	void Manager::FlipVertically()
	{
		const unsigned char bytes[2] = { Command::FLIP, 0 };
		SendToStreamingDevice( bytes, 2);
	}

	void Manager::ApplyCorrectionFilter(std::string filterName, int value)
	{
		uint8_t nameLen = static_cast<uint8_t>(filterName.size());

		std::vector<uint8_t> packet;
		packet.reserve(3 + nameLen);

		// byte 0 -> Command::Filter
		// byte 1 -> Name string length (UTF8)
		// byte 2.. -> Name string
		// byte n -> Value
		packet.push_back(Command::CORRECTION_FILTER);
		packet.push_back(nameLen);
		packet.insert(packet.end(), filterName.begin(), filterName.end());
		packet.push_back(static_cast<uint8_t>(value));

		SendToStreamingDevice( packet.data(), packet.size());
	}

	void Manager::ApplyEffectFilter(std::string filterName)
	{
		uint8_t nameLen = static_cast<uint8_t>(filterName.size());

		std::vector<uint8_t> packet;
		packet.reserve(3 + nameLen);

		// byte 0 -> Command::Filter
		// byte 1 -> Name string length (UTF8)
		// byte 2.. -> Name string
		packet.push_back(Command::EFFECT_FILTER);
		packet.push_back(nameLen);
		packet.insert(packet.end(), filterName.begin(), filterName.end());

		SendToStreamingDevice( packet.data(), packet.size());
	}

	const std::vector<DeviceDescriptor>& Manager::GetDescriptors() const
	{
		return descriptors;
	}

	const int& Manager::GetStreamingDevice() const
	{
		return streamingDevice;
	}
};