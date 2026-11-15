# VCamdroid NEXT - Status and Next Steps

Last updated: 2026-11-15 (streaming fixed)

## Current state

### STREAMING WORKS END TO END
Phone (Wi-Fi QR pairing) -> desktop preview -> live video. Verified on a Samsung SM-M536B.

Root causes found and fixed (all were protocol bugs inherited from upstream):
1. **Upstream phone parser was misaligned with the upstream desktop serializer.**
   `darusc/VCamdroid` writes every flag as a big-endian int32 but the Android app
   read four of them as single bytes and read a `focusMode` field the desktop
   never sends. The activation packet was therefore garbage, the encoder rejected
   it and the RTSP server never started. The phone now matches the desktop.
2. **Packet framing.** The phone's packet-length parser read the 16-bit lengths
   little-endian, so the filter count `7` was read as `1792` and the activation
   packet never completed (the receive buffer just grew: 152 B -> 508 B -> 760 B).
3. **Repeated activation crashed the phone.** The desktop re-sends the activation
   while there is no video; the phone re-applied the video codec, which RootEncoder
   refuses once its RTSP server runs. The phone now only sets the codec before
   the server starts and guards every option apply.
4. **The desktop blocked its own I/O thread** reading the device descriptor, which
   stalled the live connection and made the phone drop it. The descriptor is now
   read asynchronously and the control messages are framed.
5. **RTSP transport.** UDP RTP was unreliable on the network; the client now uses
   RTSP over TCP. The old vcpkg (MinGW) FFmpeg build could not open the stream at
   all, so the client is built against the MSVC FFmpeg (BtbN) now.

### Windows desktop client
- Material 3 blue/ocean theme, light + dark, optional theme mode.
- In-app pages (Settings, Connected devices, QR/preview) instead of modal dialogs.
- Material 3 popup menu, source selector, chips, sections and controls.
- Auto-selects and auto-starts a freshly connected device, and switches to the
  preview page immediately instead of staying on the QR code.
- Taskbar icon right-click menu (Open / Exit).
- All icons use exact Material icon path data.
- Stable connection ids, mutex-protected connection list, sends that cannot throw.
- RTSP handshake timeout raised to 12 s.

### Android app
- Auto-connect after a valid QR scan, with an 8 s connection watchdog.
- TCP framing, connect timeout, EOF handling, exception-safe sends.
- Preview restarted at the new resolution when the resolution changes.
- Keep screen awake while streaming.
- versionCode 209, versionName 2.0.9-NEXT.

### Packaging
- Installer: E:\Programs\Setup\VCamdroid\VCamdroid-NEXT-Setup.exe
- APK: E:\Programs\Setup\VCamdroid\VCamdroid-NEXT.apk
- Installed app: C:\Users\Admin\AppData\Local\Programs\VCamdroid NEXT

### Known / open items
- Changing the resolution interrupts the stream for a couple of seconds while the
  phone reopens the camera and encoder; the desktop shows "Restarting" meanwhile.
- The preview looked squeezed after a resolution change; the phone now reopens
  the preview at the new size. Needs one more verification round.
- StreamConfigDlg and ImgAdjDlg still use native controls (not yet M3-ified).
- DirectShow output resolution change requires an app restart.
- H.265 end-to-end not verified.
- Dropdown/popup rounded corners still need a visual pass.
- Taskbar icon cache may still show the old icon on some machines.

## Next steps (resume here)

1. Verify the resolution change: no squeeze, short interruption, picture correct.
2. Verify keep-screen-on, colours and the QR pairing flow once more.
3. Convert StreamConfigDlg and ImgAdjDlg to Material 3 pages.
4. Visual polish pass on the dropdown and context menu corners.
5. Update the GitHub issue tracker (#1 and #2) with the results.
6. Test the installer on a clean machine / MDM deployment.

## Notes
- Build commands:
  - Windows: MSBuild windows\\VCamdroid.sln /m /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:FFMPEG_ROOT=<BtbN FFmpeg root>
  - Android: cd android; gradlew.bat assembleRelease
- Key paths:
  - Source: C:\Users\Admin\AppData\Local\Temp\opencode\vcamdroid-src
  - Installed: C:\Users\Admin\AppData\Local\Programs\VCamdroid NEXT
  - Setup: E:\Programs\Setup\VCamdroid
  - Fork: https://github.com/YossefKhalil/VCamdroid-NEXT
