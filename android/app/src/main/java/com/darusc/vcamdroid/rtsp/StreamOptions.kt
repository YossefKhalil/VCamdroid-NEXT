package com.darusc.vcamdroid.rtsp

import com.darusc.vcamdroid.video.filters.FilterAdjuster
import com.darusc.vcamdroid.video.filters.FilterRepository
import com.darusc.vcamdroid.video.filters.custom.HorizontalFlipFilterRender
import com.darusc.vcamdroid.video.filters.custom.VerticalFlipFilterRender
import com.pedro.encoder.input.gl.render.filters.BaseFilterRender
import com.pedro.encoder.input.video.CameraHelper
import java.nio.ByteBuffer
import java.nio.charset.StandardCharsets

data class StreamOptions(
    var camera: CameraHelper.Facing = CameraHelper.Facing.BACK,
    var bitrate: Int = 4000 * 1024,
    var adaptiveBitrateMin: Int = 500 * 1024,
    var adaptiveBitrateMax: Int = 25000 * 1024,
    var adaptiveBitrateEnabled: Boolean = false,
    var width: Int = 640,
    var height: Int = 480,
    var fps: Int = 30,
    var rotation: Int = 0,
    var activeEffectFilter: BaseFilterRender? = null,
    var activeCorrectionFilters: MutableSet<BaseFilterRender> = HashSet(),
    var stabilization: Boolean = false,
    var focusMode: Int = 0,
    var h265Enabled: Boolean = false,
    var vFlipFilter: VerticalFlipFilterRender? = null,
    var hFlipFilter: HorizontalFlipFilterRender? = null
) {
    enum class FlipAxis { HORIZONTAL, VERTICAL }

    companion object {
        fun deserialize(data: ByteArray): StreamOptions {
            val buffer = ByteBuffer.wrap(data) // Defaults to Big Endian

            // Skip over the first byte which is packet type
            buffer.get()

            // 1. Fixed Fields
            val fps = buffer.getInt()
            val width = buffer.getInt()
            val height = buffer.getInt()

            val camera = if (buffer.getInt() == 1) CameraHelper.Facing.BACK else CameraHelper.Facing.FRONT
            // The Windows client serializes booleans as 32-bit integers (see
            // Serializer::WriteBool). Reading a single byte here shifted every
            // following field and corrupted the bitrate/filter options.
            val adaptiveBitrate = buffer.getInt() == 1

            // The Windows client stores and sends bitrates in kbps, while
            // RootEncoder expects bits per second.
            val bitrate = buffer.getInt() * 1024
            val minBitrate = buffer.getInt() * 1024
            val maxBitrate = buffer.getInt() * 1024

            val stabilization = buffer.getInt() == 1
            // Flash is applied through a separate PacketType.FLASH packet.
            buffer.getInt()
            val h265 = buffer.getInt() == 1

            // focusMode is not part of the activation packet; it defaults to auto
            // and can be changed later through PacketType.FOCUS.
            val focusMode = 0

            fun readString(): String {
                val len = buffer.short.toInt() and 0xFFFF // Unsigned short
                val bytes = ByteArray(len)
                buffer.get(bytes)
                return String(bytes, StandardCharsets.UTF_8)
            }

            // 2. Filter Sliders Map
            val filterCount = buffer.short.toInt() and 0xFFFF
            val filtersMap = mutableSetOf<BaseFilterRender>()
            repeat(filterCount) {
                val fname = readString()
                val fvalue = buffer.getInt()
                // The Windows client fills every correction filter with a
                // default value of 50. Applying those "neutral" filters still
                // pushes pixels through RootEncoder's filters, and the
                // saturation shader in particular badly corrupts colors.
                // Only activate a filter when the user actually moved it.
                if (fvalue == 50) {
                    return@repeat
                }
                FilterRepository.create(fname)?.let {
                    FilterAdjuster.adjust(it, fvalue)
                    filtersMap.add(it)
                }
            }

            // 3. Active Filters Map
            val activeEffectFilter = readString()

            return StreamOptions(
                camera,
                bitrate,
                minBitrate,
                maxBitrate,
                adaptiveBitrate,
                width,
                height,
                fps,
                0,
                FilterRepository.create(activeEffectFilter),
                filtersMap,
                stabilization,
                focusMode,
                h265,
            )
        }
    }
}