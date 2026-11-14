#include "net/serializer.h"

#include <string>
#include <stdexcept>

namespace Serializer
{
	static uint16_t ReadInt16(const uint8_t* bytes)
	{
		uint8_t b0 = static_cast<uint8_t>(bytes[0]);
		uint8_t b1 = static_cast<uint8_t>(bytes[1]);

		return (b0 << 8) | b1;
	}

	static void WriteInt16(std::vector<uint8_t>& buffer, uint16_t value)
	{
		buffer.push_back((value >> 8) & 0xFF);
		buffer.push_back(value & 0xFF);
	}

	static void WriteInt32(std::vector<uint8_t>& buffer, uint32_t value)
	{
		buffer.push_back((value >> 24) & 0xFF);
		buffer.push_back((value >> 16) & 0xFF);
		buffer.push_back((value >> 8) & 0xFF);
		buffer.push_back(value & 0xFF);
	}

	static void WriteBool(std::vector<uint8_t>& buffer, bool value)
	{
		WriteInt32(buffer, value);
	}

	static void WriteString(std::vector<uint8_t>& buffer, std::string value)
	{
		WriteInt16(buffer, static_cast<uint16_t>(value.size()));
		buffer.insert(buffer.end(), value.begin(), value.end());
	}

	// Bounds-checked reader for wire data coming from the phone. Every field
	// is validated before it is accessed so a truncated or malformed
	// descriptor cannot read past the end of the buffer.
	class Reader
	{
	public:
		Reader(const uint8_t* data, size_t size) : data(data), size(size), offset(0) {}

		uint8_t ReadByte()
		{
			Require(1);
			return data[offset++];
		}

		uint16_t ReadU16()
		{
			Require(2);
			uint8_t b0 = data[offset];
			uint8_t b1 = data[offset + 1];
			offset += 2;
			return (b0 << 8) | b1;
		}

		std::string ReadString()
		{
			const uint16_t length = ReadU16();
			Require(length);
			std::string value(reinterpret_cast<const char*>(data + offset), length);
			offset += length;
			return value;
		}

		size_t Remaining() const { return size - offset; }

	private:
		void Require(size_t count)
		{
			if (offset + count > size)
				throw std::runtime_error("Device descriptor is truncated");
		}

		const uint8_t* data;
		size_t size;
		size_t offset;
	};

	DeviceDescriptor DeserializeDeviceDescriptor(const uint8_t* bytes, size_t size)
	{
		Reader reader(bytes, size);

		// Name is the first thing
		// Each string is preceded by 2 bytes representing its size
		auto name = reader.ReadString();

		// The RTSP url follows after the name
		auto url = reader.ReadString();

		// Default protocol is udp. If it is a usb connection (via adb) switch to tcp
		std::string protocol = "udp";
		if (url.find("127.0.0.1") != std::string::npos)
		{
			protocol = "tcp";
		}

		// Represents how many resolutions are provided
		uint16_t frontResolutionCount = reader.ReadU16();

		std::vector<DeviceDescriptor::Resolution> frontResolutions;
		for (int i = 0; i < frontResolutionCount; i++)
		{
			// Each resolutions is given in pairs, first 2 bytes represents 
			// the width, the next 2 bytes represents the height
			auto w = reader.ReadU16();
			auto h = reader.ReadU16();

			frontResolutions.push_back(DeviceDescriptor::Resolution(w, h));
		}

		uint16_t backResolutionCount = reader.ReadU16();

		std::vector<DeviceDescriptor::Resolution> backResolutions;
		for (int i = 0; i < backResolutionCount; i++)
		{
			// Each resolutions is given in pairs, first 2 bytes represents 
			// the width, the next 2 bytes represents the height
			auto w = reader.ReadU16();
			auto h = reader.ReadU16();

			backResolutions.push_back(DeviceDescriptor::Resolution(w, h));
		}

		// How many filters
		uint16_t filterCount = reader.ReadU16();

		Video::Filter::Registry filters;
		for (int i = 0; i < filterCount; i++)
		{
			// First 2 bytes represent the length of the filter name,
			// followed by the actual string
			auto filterName = reader.ReadString();
			// And 1 more byte for the category
			auto cat = static_cast<Video::Filter::Category>(reader.ReadByte());

			filters[cat].push_back(filterName);
		}

		return DeviceDescriptor(name, url, protocol, frontResolutions, backResolutions, filters);
	}

	std::vector<uint8_t> SerializeStreamOptions(const StreamOptions& state)
	{
		std::vector<uint8_t> buffer;
		buffer.reserve(512);

		// First byte of the buffer is reserved
		buffer.push_back(0);

		WriteInt32(buffer, state.fps);
		WriteInt32(buffer, state.resolution.first);
		WriteInt32(buffer, state.resolution.second);

		WriteBool(buffer, state.backCameraActive);
		
		WriteBool(buffer, state.adaptiveBitrate);
		WriteInt32(buffer, state.bitrate);
		WriteInt32(buffer, state.minBitrate);
		WriteInt32(buffer, state.maxBitrate);

		WriteBool(buffer, state.stabilizationEnabled);
		WriteBool(buffer, state.flashEnabled);
		WriteBool(buffer, state.h265Enabled);

		WriteInt16(buffer, static_cast<uint16_t>(state.filterSliderValues.size()));
		for (const auto& [name, value] : state.filterSliderValues)
		{
			WriteString(buffer, name);
			WriteInt32(buffer, value);
		}

		WriteString(buffer, state.activeEffectFilter);

		return buffer;
	}

	Connection::ErrorReport DeserializeErrorReport(const uint8_t* bytes, size_t size)
	{
		Reader reader(bytes, size);

		Connection::ErrorReport report;
		report.severity = reader.ReadByte();
		report.error = reader.ReadString();
		report.description = reader.ReadString();

		return report;
	}
}