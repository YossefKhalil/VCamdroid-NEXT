package com.darusc.vcamdroid

import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.view.SurfaceHolder
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.darusc.vcamdroid.databinding.ActivityStreamBinding
import com.darusc.vcamdroid.networking.ConnectionManager
import com.darusc.vcamdroid.networking.PacketType
import com.darusc.vcamdroid.rtsp.Streamer
import com.darusc.vcamdroid.rtsp.StreamOptions
import com.darusc.vcamdroid.util.Logger
import com.pedro.encoder.utils.gl.AspectRatioMode
import java.io.ByteArrayOutputStream

class StreamActivity : AppCompatActivity(), SurfaceHolder.Callback, ConnectionManager.ConnectionStateCallback {

    private val TAG = "VCamdroid"

    private companion object {
        const val NEED_MORE_BYTES = -1
        const val INVALID_PACKET = -2
    }

    private lateinit var viewBinding: ActivityStreamBinding

    private val connectionManager = ConnectionManager.getInstance(this)
    private lateinit var streamer: Streamer

    // TCP is a byte stream: one read can contain a partial packet or several
    // coalesced packets. Accumulate bytes and only dispatch complete packets.
    private val receiveAccumulator = ByteArrayOutputStream()

    // ACTIVATION can arrive before the SurfaceView is ready. Hold the options
    // and start the stream from surfaceCreated instead of racing the EGL setup.
    private var pendingActivation: StreamOptions? = null

    private lateinit var statusView: TextView

    private fun setStatus(text: String) {
        runOnUiThread {
            if (::statusView.isInitialized) statusView.text = text
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        viewBinding = ActivityStreamBinding.inflate(layoutInflater)
        viewBinding.root.keepScreenOn = true
        viewBinding.surfaceView.setAspectRatioMode(AspectRatioMode.Adjust)

        setContentView(viewBinding.root)

        // On-screen state indicator so connection/activation problems are
        // visible on the phone without pulling a log file.
        statusView = TextView(this).apply {
            setTextColor(Color.WHITE)
            setBackgroundColor(Color.argb(170, 0, 0, 0))
            textSize = 13f
            setPadding(32, 24, 32, 24)
            text = "Waiting for activation..."
        }
        viewBinding.root.addView(statusView, ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT))

        // Create the streamer before registering the byte callback: the
        // callback may fire immediately with a buffered ACTIVATION packet.
        streamer = Streamer(StreamOptions(), this, viewBinding.surfaceView)
        streamer.statusCallback = { setStatus(it) }
        connectionManager.setOnBytesReceivedCallback(::onBytesReceived)

        // Register after the streamer exists. If the surface is already
        // available this invokes surfaceCreated immediately.
        viewBinding.surfaceView.holder.addCallback(this)

        // Disable navigating to logs activity for now as it breaks streaming
//        viewBinding.logReportButton.setOnClickListener {
//            val intent = Intent(this, LogActivity::class.java)
//            startActivity(intent)
//        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        if (holder.surface != null && holder.surface.isValid) {
            setStatus("Surface ready")
            streamer.startPreview()
            pendingActivation?.let {
                setStatus("Starting pending stream...")
                pendingActivation = null
                streamer.startStream(it)
            }
        }
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) { }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        streamer.stop()
    }

    override fun onDisconnected() {
        Toast.makeText(this, "Connection closed by server", Toast.LENGTH_LONG).show()
        Logger.log("STREAM", "TCP server disconnected")
        streamer.stop()
        finish()
    }

    private fun onBytesReceived(buffer: ByteArray, bytes: Int) {
        if (bytes <= 0) return
        receiveAccumulator.write(buffer, 0, bytes)
        setStatus("RX $bytes B (total ${receiveAccumulator.size()} B)")

        var pending = receiveAccumulator.toByteArray()
        while (pending.isNotEmpty()) {
            when (val length = getPacketLength(pending)) {
                NEED_MORE_BYTES -> break
                INVALID_PACKET -> {
                    // Drop one byte and try to resynchronize instead of
                    // spinning forever on a corrupt header.
                    pending = pending.copyOfRange(1, pending.size)
                }
                else -> {
                    if (pending.size < length) break
                    handlePacket(pending.copyOfRange(0, length))
                    pending = pending.copyOfRange(length, pending.size)
                }
            }
        }

        receiveAccumulator.reset()
        if (pending.isNotEmpty()) receiveAccumulator.write(pending)
    }

    private fun handlePacket(buffer: ByteArray) {
        // Packet type is always the first byte
        val type = buffer[0]
        setStatus("Packet type 0x${type.toString(16)} (${buffer.size} B)")

        when (type) {
            PacketType.ACTIVATION -> {
                setStatus("Activation received, starting stream...")
                val options = StreamOptions.deserialize(buffer)
                if (viewBinding.surfaceView.holder.surface?.isValid == true) {
                    streamer.startStream(options)
                } else {
                    pendingActivation = options
                }
            }
            PacketType.CAMERA -> {
                streamer.switchCamera()
            }
            PacketType.RESOLUTION -> {
                val width = (buffer[1].toInt() and 0xFF) or (buffer[2].toInt() shl 8)
                val height = (buffer[3].toInt() and 0xFF) or (buffer[4].toInt() shl 8)
                streamer.setResolution(width, height)
            }
            PacketType.ROTATION -> {
                val degrees = buffer[1].toInt();
                streamer.rotate(degrees)
            }
            PacketType.EFFECT_FILTER -> {
                val filterName = String(buffer, 2, buffer[1].toInt(), Charsets.UTF_8)
                streamer.applyEffectFilter(filterName)
            }
            PacketType.CORRECTION_FILTER -> {
                val filterName = String(buffer, 2, buffer[1].toInt(), Charsets.UTF_8)
                val value = buffer[2 + buffer[1].toInt()].toInt()
                streamer.applyCorrectionFilter(filterName, value)
            }
            PacketType.BITRATE -> {
                val bitrate = (buffer[1].toInt() and 0xFF) or (buffer[2].toInt() shl 8)
                streamer.setBitrate(bitrate)
            }
            PacketType.ADAPTIVE_BITRATE ->  {
                val min = (buffer[1].toInt() and 0xFF) or (buffer[2].toInt() shl 8)
                val max = (buffer[3].toInt() and 0xFF) or (buffer[4].toInt() shl 8)
                streamer.setAdaptiveBitrate(min, max)
            }
            PacketType.STABILIZATION -> {
                streamer.setStabilization(buffer[1].toInt() == 1)
            }
            PacketType.FLASH -> {
                streamer.setFlash(buffer[1].toInt() == 1)
            }
            PacketType.FOCUS -> {
                streamer.setFocus(buffer[1].toInt())
            }
            PacketType.CODEC -> {
                streamer.setH265Codec(buffer[1].toInt() == 1)
            }
            PacketType.FPS -> {
                streamer.setFps(buffer[1].toInt())
            }
            PacketType.ZOOM -> {
                val factor = (buffer[1].toInt() and 0xFF) or
                        ((buffer[2].toInt() and 0xFF) shl 8) or
                        ((buffer[3].toInt() and 0xFF) shl 16) or
                        ((buffer[4].toInt() and 0xFF) shl 24)
                streamer.setZoom(Float.fromBits(factor))
            }
            PacketType.FLIP -> {
                val axis = buffer[1].toInt()
                streamer.flip(if (axis == 0) StreamOptions.FlipAxis.VERTICAL else StreamOptions.FlipAxis.HORIZONTAL)
            }
        }
    }

    /**
     * Returns the total length of the packet at the start of [data], or one of
     * the NEED_MORE_BYTES / INVALID_PACKET sentinels. Control packets have no
     * length prefix, so the length is derived from the packet type and, for
     * variable-length packets, from their embedded string lengths.
     */
    private fun getPacketLength(data: ByteArray): Int {
        if (data.isEmpty()) return NEED_MORE_BYTES

        return when (data[0]) {
            PacketType.ACTIVATION -> {
                // 1 type byte + 11 int32 fields + u16 filter count
                if (data.size < 47) return NEED_MORE_BYTES

                var offset = 47
                val filterCount = readU16(data, 45)
                repeat(filterCount) {
                    if (data.size < offset + 2) return NEED_MORE_BYTES
                    val nameLength = readU16(data, offset)
                    offset += 2
                    // name + int32 slider value
                    if (data.size < offset + nameLength + 4) return NEED_MORE_BYTES
                    offset += nameLength + 4
                }

                if (data.size < offset + 2) return NEED_MORE_BYTES
                val effectLength = readU16(data, offset)
                offset += 2
                if (data.size < offset + effectLength) return NEED_MORE_BYTES
                offset + effectLength
            }
            PacketType.RESOLUTION -> 5
            PacketType.CAMERA -> 1
            PacketType.QUALITY -> 2
            PacketType.CORRECTION_FILTER -> {
                if (data.size < 2) NEED_MORE_BYTES
                else {
                    val length = 3 + (data[1].toInt() and 0xFF)
                    if (data.size < length) NEED_MORE_BYTES else length
                }
            }
            PacketType.EFFECT_FILTER -> {
                if (data.size < 2) NEED_MORE_BYTES
                else {
                    val length = 2 + (data[1].toInt() and 0xFF)
                    if (data.size < length) NEED_MORE_BYTES else length
                }
            }
            PacketType.ROTATION,
            PacketType.STABILIZATION,
            PacketType.FLASH,
            PacketType.FOCUS,
            PacketType.CODEC,
            PacketType.FPS,
            PacketType.FLIP -> 2
            PacketType.BITRATE -> 3
            PacketType.ADAPTIVE_BITRATE,
            PacketType.ZOOM -> 5
            else -> INVALID_PACKET
        }
    }

    private fun readU16(data: ByteArray, offset: Int): Int =
        ((data[offset].toInt() and 0xFF) shl 8) or (data[offset + 1].toInt() and 0xFF)
}