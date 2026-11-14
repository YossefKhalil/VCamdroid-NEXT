<h1 align="center">
  <sub>
    <img src="imgs/icon2.png" width="150">
  </sub>
  <br>
  VCamdroid NEXT
</h1>

<p align="center">Turn your Android phone into a high-performance Windows webcam.</p>

<p align="center">
  <a href="https://github.com/YossefKhalil/VCamdroid-NEXT/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/YossefKhalil/VCamdroid-NEXT?style=for-the-badge" alt="License">
  </a>
  <a href="https://github.com/YossefKhalil/VCamdroid-NEXT/releases">
    <img src="https://img.shields.io/github/v/release/YossefKhalil/VCamdroid-NEXT?style=for-the-badge" alt="Release">
  </a>
</p>

## Table of Contents

1. [**Description**](#description)
2. [**What's new in NEXT**](#whats-new-in-next)
3. [**Key Features**](#key-features)
4. [**Installation Guide**](#installation-guide)
5. [**Usage Instructions**](#usage-instructions)
6. [**Troubleshooting**](#troubleshooting)
7. [**Reporting Issues**](#reporting-issues)
8. [**Technical Architecture**](#technical-architecture)
9. [**Contributing**](#contributing)

## Description

VCamdroid NEXT is a maintained fork of VCamdroid that focuses on stability, protocol robustness and a modern desktop experience. It turns your mobile device's camera into a virtual webcam for Windows using a custom DirectShow filter provided by the [Softcam library](https://github.com/tshino/softcam), so it works with Zoom, OBS, Discord, Teams and any other application that supports standard webcams.

Connect over USB for the lowest latency, or over Wi-Fi with the built-in QR code pairing. The stream is hardware-encoded on the phone and decoded on the PC for a smooth, low-overhead video feed.

<p align="center">
  <img src="imgs/demo.gif" width="600" alt="VCamdroid NEXT Demo">
</p>

## What's new in NEXT

VCamdroid NEXT is version **2.0.0-NEXT**. It is built from the original 1.3.x codebase with the fixes and polish collected below.

### Desktop client (Windows)

* **Material 3 interface.** The whole client was rebuilt around the Material 3 design language: a light and dark blue/ocean colour scheme, cards, tonal controls, rounded surfaces and a proper type scale.
* **Optional theme.** Follow the system theme, or lock the client to Light or Dark.
* **QR code is always ready.** The main page shows the pairing QR code immediately, so no extra "Connect" step is needed.
* **In-app pages instead of modal windows.** Settings and Connected devices open as normal pages with a back button, which also removes the crash-on-close that affected the old dialogs.
* **Short, focused menu.** The overflow menu only contains the essentials; stream settings and image adjustments live in the toolbar, and everything else moved into the Settings page.
* **Crash-on-connect fixes.** Device callbacks are marshalled onto the UI thread, descriptor parsing is bounds-checked, the TCP server starts after the window exists, and disconnects always close the socket cleanly.
* **Dynamic codec support.** The RTSP receiver now picks the decoder from the negotiated codec, so H.264 and H.265/HEVC both work.
* **Aspect-ratio correct preview.** The preview canvas adapts to the real window size and letterboxes the stream instead of stretching it.
* **Colour fix.** The client no longer seeds every correction filter at its neutral value, which previously caused extreme saturation/colour corruption until the resolution was changed.
* **New app icon.** A modern frosted Material 3 webcam icon, in circular form, used for the window, taskbar and installer.

### Android app

* **Auto-connect after QR scan.** Scanning a valid QR code connects immediately; the old "Connect to this IP?" dialog is gone.
* **Correct preview aspect ratio.** The preview view is no longer forced into a 3:4 box and now uses fit/letterbox scaling.
* **Robust TCP framing.** An accumulator reassembles split or coalesced control packets so activation and resolution changes are not lost.
* **Keep screen awake.** The phone stays on while streaming.
* **Neutral filter fix.** Default colour-correction values are skipped, and the remaining slider mappings were corrected so colours stay accurate.
* **Version 2.0.0-NEXT**, with a new version code (200).

## Key Features

* **Universal Compatibility:** Works with any Windows application that supports standard webcams.
* **Flexible Connectivity:** High-speed wired connections over ADB, or convenient wireless connections over Wi-Fi.
* **Multi-Device Support:** Connect multiple Android devices and switch between them instantly.
* **Full Camera Control:** Toggle between front and back cameras, change resolution, adjust bitrate, and enable the flash.
* **Image Adjustments:** Real-time rotation, mirroring (flip), brightness, contrast, saturation and other filters.
* **Zero-Config Pairing:** Automatic USB connection through ADB, and instant QR-code pairing for Wi-Fi.
* **Modern Desktop UX:** A Material 3 client with a light/dark theme, in-app settings, and a live preview.

## Installation Guide

### Prerequisites
* **PC:** Windows 10 or 11 (x64).
* **Phone:** Android 7.0 (Nougat) or higher.

### Step 1: Install on Windows
1. Download the latest **VCamdroid NEXT** installer from the [Releases Page](https://github.com/YossefKhalil/VCamdroid-NEXT/releases), or run `VCamdroid-NEXT-Setup.exe`.
2. Run the installer. It registers `softcam.dll` with the system so the virtual webcam becomes visible to other applications.
3. When the Windows Firewall prompt appears, allow the app on both **Private** and **Public** networks.

> If you install the ZIP build manually, run `install.bat` as Administrator, or register the filter with `regsvr32 softcam.dll`.

### Step 2: Install on Android
Install `VCamdroid-NEXT.apk` on your phone. You can copy it from the installer's `apk` folder, or use `install_apk.bat` with USB debugging enabled to install it automatically.

### 💡 How to Enable USB Debugging
1. Go to **Settings > About Phone**.
2. Find **Build Number** and tap it **7 times** until you see "You are now a developer!"
3. Go back to **Settings > System > Developer Options**.
4. Toggle **USB Debugging** to **ON**.
    * *For device-specific steps, refer to the [official Android documentation](https://developer.android.com/studio/debug/dev-options).*

## Usage Instructions

### Wired Connection (USB / ADB)
*Recommended for lowest latency and highest stability.*

1. Connect your phone to the PC via USB.
2. Launch the **VCamdroid NEXT** desktop client.
3. Launch the **VCamdroid NEXT** app on your phone.
4. The connection is automatic. The phone switches to streaming mode and the preview appears on the PC.

### Wireless Connection (Wi-Fi)
1. Ensure both your PC and phone are on the same Wi-Fi network.
2. Launch the **VCamdroid NEXT** desktop client. The pairing QR code is already shown on the main page.
3. Launch the **VCamdroid NEXT** app on your phone.
4. Point the camera at the PC screen. The app connects automatically once the QR code is recognised.

If you encounter any problems check the [**Troubleshooting**](#troubleshooting) section. If the issue still persists, please [report the issue](#reporting-issues).

---

## Technical Architecture

### Networking Protocol
VCamdroid NEXT uses the industry-standard **RTSP (Real-Time Streaming Protocol)** for robust, low-latency video transmission between the Android device and the Windows client.

1. **Transport Layer:**
    * **Wi-Fi Connection:** The Windows client connects directly to the RTSP server running on the Android device over the local network.
    * **USB Connection:** Wired communication uses **ADB Port Forwarding**. Since ADB only supports TCP forwarding, the RTSP stream is tunnelled over **TCP** (interleaved RTSP), creating a stable high-bandwidth path through `localhost`.
2. **Stream Handling Libraries:**
    * **Server Side (Android):** Powered by the [RootEncoder](https://github.com/pedroSG94/RootEncoder) library. It interfaces with the Android encoder, packetises the video into RTP packets, and manages the RTSP server session.
    * **Client Side (Windows):** Uses [FFmpeg](https://ffmpeg.org/) to demux the RTSP stream and decode the incoming video packets. The decoder is selected dynamically from the negotiated codec, so both H.264 and H.265/HEVC are supported.

<p align="center"><img src="imgs/network.png" width="60%"></p>

### Video Pipeline
The pipeline is engineered for performance, offloading image processing to the Android GPU before compression to minimise latency and bandwidth.

1. **Capture, Process & Encode (Android):**
    * **Capture:** Video frames are captured with the modern **Camera2 API**.
    * **Pre-Processing:** Frames are processed on the GPU with OpenGL. Rotation, mirroring and colour correction are applied before encoding.
    * **Hardware Encoding:** The processed frames are passed to the device's hardware **MediaCodec** (H.264 or H.265/HEVC), offloading compression from the CPU.
    * **Transmission:** **RootEncoder** packetises the encoded stream and transmits it over the active connection.
2. **Decode & Render (Windows):**
    * **Decoding:** The Windows client receives the RTSP stream and uses **FFmpeg** to decode the compressed frames into raw image data.
    * **Output:**
        * **UI Preview:** The decoded frame is rendered in the client window with correct aspect-ratio scaling.
        * **Virtual Device:** The frame is written to a ring buffer managed by the [Softcam](https://github.com/tshino/softcam) library, which bridges the client and the system-registered DirectShow filter so third-party apps can use the stream as a physical webcam.

<p align="center"><img src="imgs/pipeline.png" width="50%"></p>

## 🤝 Contributing

Contributions are welcome. Whether you are fixing a bug, improving performance, or adding a feature, feel free to fork the repository and submit a Pull Request.

### 📂 Repository Structure
* `android/`: The Android Studio project (Kotlin). Handles Camera2 API capture, OpenGL processing and RTSP streaming.
* `windows/`: The Visual Studio solution (C++). Contains the Desktop Client GUI and the DirectShow filter integration.

---

### 🛠️ Development Setup

#### 📱 Android App
1. **Prerequisites:** Install the latest **Android Studio**.
2. **Import:** Open the `android/` directory as a project.
3. **Build:** Let Gradle sync and download dependencies.
    * *Core Dependency:* [RootEncoder](https://github.com/pedroSG94/RootEncoder) (handles RTSP/RTP packets).
4. **Run:** Connect a physical Android device (emulators often lack the necessary encoder hardware) and run the `app` module.

#### 💻 Windows Client
1. **Prerequisites:**
    * **Visual Studio 2022** with the "Desktop development with C++" workload (the project targets the **v143** toolset).
    * **vcpkg** for `asio`, `wxWidgets 3.3.x` and `ffmpeg`.
2. **Install Dependencies:**
    ```powershell
    vcpkg install wxwidgets:x64-windows ffmpeg:x64-windows
    vcpkg integrate install
    ```
3. **Build Instructions:**
    * Build `softcam`. See the [Softcam repository](https://github.com/tshino/softcam?tab=readme-ov-file#how-to-build-the-library) for instructions.
    * Open `windows/VCamdroid.sln`.
    * Set the configuration to **Release / x64** (x86 is not supported).
    * Build the solution.
4. **Testing the Driver:**
    * The DirectShow filter (`softcam.dll`) must be registered to be visible to apps like OBS or Zoom.
    * Run `install.bat` as Administrator in your output directory, or register it manually:
        ```cmd
        regsvr32 softcam.dll
        ```

### 📬 Submitting a Pull Request
1. Fork the project.
2. Create your feature branch (`git checkout -b feature/AmazingFeature`).
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`).
4. Push to the branch (`git push origin feature/AmazingFeature`).
5. Open a Pull Request.

## Troubleshooting

### App Crashes / "VCRUNTIME140.dll was not found"
If the application closes immediately or you see a system popup about missing DLLs (like `VCRUNTIME140.dll` or `MSVCP140.dll`), your PC is missing the C++ runtime libraries.
* **Fix:** Download and install the [latest VC++ Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe) from Microsoft.

### Connection Failed / Host Unreachable
If the Android app cannot connect to the Windows client:
1. **Check Windows Firewall:** The firewall often blocks incoming streams.
    * Search for **"Allow an app through Windows Firewall"** in the Start Menu.
    * Find `VCamdroid NEXT.exe` in the list and ensure both **Private** and **Public** boxes are checked.
2. **Verify Network Visibility (Reverse Ping Test):**
    * Connect via USB (for the test command).
    * Open a terminal in the VCamdroid NEXT folder and run: `adb shell ping -c 4 <PC_IP_ADDRESS>`
    * If you see "100% packet loss" or "unreachable," your PC's firewall or router settings (AP Isolation) are blocking the connection.

### The app installs but does not replace the old VCamdroid
VCamdroid NEXT uses a new application folder (`%LOCALAPPDATA%\Programs\VCamdroid NEXT`) and a new executable name. Uninstall the old **VCamdroid** entry from Windows if you no longer need it.

### The Android app is rejected by MDM with "version code already exists"
VCamdroid NEXT ships with a new version code (**200**). If your MDM still reports an older code, make sure you are uploading `VCamdroid-NEXT.apk` and not an older APK.

### USB Connection not working
If the app does not detect your phone:
1. **Check ADB Devices:**
    * Open a terminal in the VCamdroid NEXT folder.
    * Run: `adb devices`
    * **If the list is empty:** Your cable is bad or the [Universal ADB Drivers](https://adb.clockworkmod.com/) are missing.
    * **If "unauthorized":** Check your phone screen and tap "Allow" on the **"Allow USB Debugging?"** popup.
2. **Kill Conflicting ADB Processes:**
    * Open **Task Manager** (`Ctrl + Shift + Esc`).
    * Search for `adb.exe` in the **Details** tab.
    * Right-click and select **End Task**, then restart VCamdroid NEXT.

---

## Reporting Issues

VCamdroid NEXT is a community fork and hardware compatibility varies across thousands of Android devices. Your feedback is crucial.

If you encounter a bug or crash, please open a [New Issue](https://github.com/YossefKhalil/VCamdroid-NEXT/issues) and attach the logs so we can fix it faster.

### How to get the logs:
1. **Android Logs:**
    * Open the VCamdroid NEXT app on your phone.
    * Tap the **Bug Icon** 🐞 in the top corner.
    * Click the **Save/Share** button to export the log file.
2. **Windows Logs:**
    * Check the `vcamdroid.log` file inside the VCamdroid NEXT installation directory.
    * Copy the text from the latest log file.

**Please include:**
* Phone Model (e.g., Samsung S21, Pixel 6)
* Android Version
* Connection Method (USB or Wi-Fi)
* VCamdroid NEXT version (2.0.0-NEXT)
