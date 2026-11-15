package com.darusc.vcamdroid.networking

import android.util.Log
import com.darusc.vcamdroid.networking.connection.Connection
import com.darusc.vcamdroid.networking.connection.TCPConnection
import com.darusc.vcamdroid.util.Logger
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class ConnectionManager private constructor() : Connection.Listener {

    private val TAG = "VCamdroid"

    companion object {
        @Volatile private var instance: ConnectionManager? = null

        fun getInstance(): ConnectionManager {
            synchronized(this) {
                return instance ?: ConnectionManager().also { instance = it }
            }
        }

        fun getInstance(connectionStateCallback: ConnectionStateCallback): ConnectionManager {
            synchronized(this) {
                if(instance == null) {
                    instance = ConnectionManager()
                }
                instance!!.setConnectionStateCallback(connectionStateCallback)
                return instance!!
            }
        }
    }

    enum class Mode {
        USB,
        WIFI
    }

    interface ConnectionStateCallback {
        fun onConnectionSuccessful(connectionMode: Mode) { }
        fun onConnectionFailed(connectionMode: Mode) { }
        fun onDisconnected() { }
    }

    val localIpAddress: String
        get() = tcpConn!!.localIpAddress

    private var connectionStateCallback: ConnectionStateCallback? = null
    private var onBytesReceivedCallback: ((buffer: ByteArray, bytes: Int) -> Unit)? = null
    private val pendingBytes = java.io.ByteArrayOutputStream()
    private val pendingLock = Any()

    // The desktop expects the device descriptor as the very first message on
    // the control connection. Status reports are queued until it was sent.
    private var descriptorSent = false
    private val pendingReports = ArrayList<com.darusc.vcamdroid.networking.ErrorReport>()
    private val reportLock = Any()

    private var tcpConn: TCPConnection? = null

    private var streamingEnabled = false

    private fun setConnectionStateCallback(connectionStateCallback: ConnectionStateCallback) {
        this.connectionStateCallback = connectionStateCallback
    }

    /**
     * The bytesReceivedListener is called only for packets of type different than PacketType.ACTIVATION.
     * These packets are handled internally
     */
    fun setOnBytesReceivedCallback(onBytesReceivedCallback: ((buffer: ByteArray, bytes: Int) -> Unit)?) {
        this.onBytesReceivedCallback = onBytesReceivedCallback
        // Flush any bytes that arrived before the callback was registered (race
        // between the desktop's activation packet and StreamActivity creation).
        if (onBytesReceivedCallback != null) {
            synchronized(pendingLock) {
                if (pendingBytes.size() > 0) {
                    val buffered = pendingBytes.toByteArray()
                    pendingBytes.reset()
                    onBytesReceivedCallback(buffered, buffered.size)
                }
            }
        }
    }

    /**
     * Connect in WIFI mode.
     */
    fun connect(ipAddress: String, port: Int) {
        synchronized(reportLock) {
            descriptorSent = false
            pendingReports.clear()
        }
        CoroutineScope(Dispatchers.IO).launch {
            try {
                tcpConn = TCPConnection(ipAddress, port, false, this@ConnectionManager)
                connectionStateCallback?.onConnectionSuccessful(Mode.WIFI)
            } catch (e: Connection.ConnectionFailedException) {
                Logger.log("CONNECTION MANAGER", "" + e.message)
                connectionStateCallback?.onConnectionFailed(Mode.WIFI)
            }
        }
    }

    /**
     * Connect in USB mode (through adb)
     */
    fun connect(port: Int) {
        CoroutineScope(Dispatchers.IO).launch {
            try {
                tcpConn = TCPConnection("127.0.0.1", port, true, this@ConnectionManager)
                connectionStateCallback?.onConnectionSuccessful(Mode.USB)
            } catch (e: Connection.ConnectionFailedException) {
                Logger.log("CONNECTION MANAGER", "" + e.message)
                connectionStateCallback?.onConnectionFailed(Mode.USB)
            }
        }
    }

    fun sendDescriptor(descriptor: DeviceDescriptor) {
        CoroutineScope(Dispatchers.IO).launch {
            try {
                tcpConn?.send(descriptor.serialize())
            } catch (e: Exception) {
                Logger.log("CONNECTION MANAGER", "Failed to send descriptor: ${e.message}")
                return@launch
            }

            // The descriptor must be the first message on the wire. Anything
            // that was queued in the meantime is flushed now.
            val queued = synchronized(reportLock) {
                descriptorSent = true
                val copy = ArrayList(pendingReports)
                pendingReports.clear()
                copy
            }
            for (report in queued) {
                try {
                    tcpConn?.send(report.serialize())
                } catch (e: Exception) {
                    Logger.log("CONNECTION MANAGER", "Failed to send report: ${e.message}")
                }
            }
        }
    }

    fun sendErrorReport(report: ErrorReport) {
        CoroutineScope(Dispatchers.IO).launch {
            try {
                val sendNow = synchronized(reportLock) {
                    if (!descriptorSent) {
                        pendingReports.add(report)
                        false
                    } else {
                        true
                    }
                }
                if (sendNow) {
                    tcpConn?.send(report.serialize())
                }
            } catch (e: Exception) {
                // A broken pipe must never take the app down.
                Logger.log("CONNECTION MANAGER", "Failed to send report: ${e.message}")
            }
        }
    }

    override fun onBytesReceived(buffer: ByteArray, bytes: Int) {
        val cb = onBytesReceivedCallback
        if (cb != null) {
            cb(buffer, bytes)
        } else {
            // Buffer until StreamActivity registers its handler.
            synchronized(pendingLock) {
                pendingBytes.write(buffer, 0, bytes)
            }
        }
    }

    override fun onDisconnected() {
        CoroutineScope(Dispatchers.Main).launch {
            streamingEnabled = false
            tcpConn?.close()
            connectionStateCallback?.onDisconnected()
        }
    }
}