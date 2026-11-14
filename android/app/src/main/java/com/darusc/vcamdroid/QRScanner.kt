package com.darusc.vcamdroid

import android.graphics.Rect
import android.util.Log
import androidx.annotation.OptIn
import androidx.camera.core.ExperimentalGetImage
import androidx.camera.core.ImageProxy
import com.google.mlkit.vision.barcode.BarcodeScanner
import com.google.mlkit.vision.barcode.BarcodeScannerOptions
import com.google.mlkit.vision.barcode.BarcodeScanning
import com.google.mlkit.vision.barcode.common.Barcode
import com.google.mlkit.vision.common.InputImage

class QRScanner() {

    data class Result(
        var address: String,
        var port: Int
    )

    private var options: BarcodeScannerOptions
    private var scanner: BarcodeScanner

    private var addressRegex = Regex("""\b((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\b""")

    private var enabled = false

    init {
        options = BarcodeScannerOptions.Builder().setBarcodeFormats(Barcode.FORMAT_QR_CODE).build()
        scanner = BarcodeScanning.getClient(options)
    }

    fun start() {
        enabled = true
    }

    fun stop() {
        enabled = false
    }

    @OptIn(ExperimentalGetImage::class)
    fun launchScanTask(imageProxy: ImageProxy, rect: Rect, callback: (QRScanner.Result?) -> Unit) {
        if(!enabled) {
            imageProxy.close()
            return
        }

        val mediaImage = imageProxy.image
        if(mediaImage != null) {
            val image = InputImage.fromMediaImage(mediaImage, imageProxy.imageInfo.rotationDegrees)
            val result = scanner.process(image)
                .addOnSuccessListener { barcodes ->
                    // Accept a valid QR anywhere in the frame. The overlay is only a
                    // visual guide; requiring the bounding box to be fully inside it
                    // made ML Kit silently ignore perfectly readable codes.
                    for (barcode in barcodes) {
                        val parsed = parseResult(barcode.rawValue ?: "")
                        if (parsed != null) {
                            // Stop the scanner. Further attempts at connecting should be
                            // manually triggered otherwise multiple connections might be
                            // established (for each frame of the camera).
                            stop()
                            callback(parsed)
                            imageProxy.close()
                            return@addOnSuccessListener
                        }
                    }
                    imageProxy.close()
                }
                .addOnFailureListener {
                    imageProxy.close()
                }
        } else {
            imageProxy.close()
        }
    }

    private fun parseResult(value: String): Result? {
        val splits = value.trim().split(":")
        if(splits.size != 2) {
            return null
        }

        val address = splits[0].trim()
        val port = splits[1].trim().toIntOrNull() ?: return null
        if(!address.matches(addressRegex) || port !in 1..65535) {
            return null
        }

        return Result(address, port)
    }
}