# VCamdroid NEXT - Status and Next Steps

Last updated: 2026-11-14 (latest connectivity & stability pass)

## Current state

### Windows desktop client
- Material 3 blue/ocean theme, light + dark, with optional theme mode (Follow system / Light / Dark).
- In-app pages instead of modal dialogs: Settings, Connected devices, QR/preview (simplebook).
- Settings page is scrollable and uses M3 rows (radio/switch) instead of native controls.
- Connected devices page uses custom device cards and an empty state.
- Material 3 source selector (Select) instead of native wxChoice.
- Material 3 popup menu (short: Connected devices, Settings, About, Exit).
- Stream settings moved to toolbar (gear icon); image adjustments (sliders icon) separate.
- QR code shown directly on the main page, ready to scan.
- Status and zoom shown as M3 chips.
- In-app title bar logo loaded from a high-quality circular PNG.
- Auto-switch to a freshly connected device when nothing is streaming.
- Crash-on-connect fixes: CallAfter marshalling, Connection::Read always Close, descriptor bounds checks, server starts after window.
- Dynamic H.264/H.265 decoder selection.
- Dynamic preview canvas sizing with aspect-ratio letterboxing.
- Colour-corruption fix: neutral (50) correction filters are skipped and slider mappings corrected.
- New circular app icon embedded in the exe (VCamdroid NEXT.exe).
- Version identity: VCamdroid NEXT, 2.0.0-NEXT.

### Latest changes (this pass)
- Auto-start the stream immediately when a device connects, and switch the UI to the preview page right away (no more lingering on the QR page).
- Fixed the broken auto-select: OnDeviceConnected now calls StartStreamForDevice directly instead of routing a synthetic event through the main window.
- Added Manager::Disconnect() and repaired descriptor/connection indices on disconnect, so a dropped phone stops retrying and a later reconnect auto-starts cleanly.
- Thread-safety for Server::connections (mutex) and Connection lifetime (shared_ptr kept alive during async reads, guarded Close).
- Connection::Send no longer throws; socket errors are logged.
- RTSP receiver logs the full open-input error and URL.
- Window::ShowPreviewPage added.
- Taskbar icon now has a right-click context menu (Open / Exit).
- Standard Material Icons now drawn from exact Material path data (arrow_back, rotate_left/right, qr_code, devices, settings, tune, more_vert, flip, zoom, flash, swap_camera, photo_camera, close, check, info, logout, bar_chart, expand_more, refresh).

### Android app
- Auto-connect immediately after a valid QR scan.
- Preview aspect ratio fixed (match_parent + AspectRatioMode.Adjust).
- TCP receive accumulator for split/coalesced packets.
- Keep screen awake while streaming.
- Neutral correction-filter values skipped (colour fix).
- New circular launcher icons and "VCamdroid NEXT" label.
- versionCode 200, versionName 2.0.0-NEXT.

### Packaging
- Installer: E:\Programs\Setup\VCamdroid\VCamdroid-NEXT-Setup.exe
- APK: E:\Programs\Setup\VCamdroid\VCamdroid-NEXT.apk
- Installed app: C:\Users\Admin\AppData\Local\Programs\VCamdroid
- Setup icon and app icon refreshed.

### Known / open items
- "Connecting and nothing happens" after phone connects: auto-switch fixed in code; needs live phone verification.
- StreamConfigDlg and ImgAdjDlg still use native controls (not yet M3-ified).
- DirectShow output resolution change requires an app restart (noted in UI).
- H.265 end-to-end not verified.
- Installer uses admin privileges with per-user install dir (Inno warning).
- Android launcher icon renders too zoomed-in/cropped; needs a less tight icon mask.
- Desktop in-app page back button and rotate buttons: now use standard Material icons.
- Dropdown/popup window shape glitches: window-region + DWM disable applied; visual verification pending.
- Taskbar icon right-click menu added.

## Next steps (resume here)

1. Test end-to-end on the phone: install APK, run desktop client, scan QR, verify auto-connect, preview page switch, stream starts, keep-awake.
2. If the stream still does not start, check the RTSP receiver log (vcamdroid.log) for the URL and open-input error.
3. Convert StreamConfigDlg and ImgAdjDlg to Material 3 pages or dialogs (remove native controls).
4. Visual polish: dropdown/popup rounded corners, taskbar icon cache.
5. Update the GitHub issue tracker (#1 and #2) with verification results.
6. Test the installer on a clean machine / MDM deployment.

## Notes
- Build commands:
  - Windows: MSBuild windows\\VCamdroid.sln /m /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:FFMPEG_ROOT=...
  - Android: cd android; gradlew.bat assembleRelease
- Key paths:
  - Source: C:\Users\Admin\AppData\Local\Temp\opencode\vcamdroid-src
  - Installed: C:\Users\Admin\AppData\Local\Programs\VCamdroid
  - Setup: E:\Programs\Setup\VCamdroid
  - Fork: https://github.com/YossefKhalil/VCamdroid-NEXT


## Current state

### Windows desktop client
- Material 3 blue/ocean theme, light + dark, with optional theme mode (Follow system / Light / Dark).
- In-app pages instead of modal dialogs: Settings, Connected devices, QR/preview (simplebook).
- Settings page is scrollable and uses M3 rows (radio/switch) instead of native controls.
- Connected devices page uses custom device cards and an empty state.
- Material 3 source selector (Select) instead of native wxChoice.
- Material 3 popup menu (short: Connected devices, Settings, About, Exit).
- Stream settings moved to toolbar (gear icon); image adjustments (sliders icon) separate.
- QR code shown directly on the main page, ready to scan.
- Status and zoom shown as M3 chips.
- In-app title bar logo loaded from a high-quality circular PNG.
- Auto-switch to a freshly connected device when nothing is streaming.
- Crash-on-connect fixes: CallAfter marshalling, Connection::Read always Close, descriptor bounds checks, server starts after window.
- Dynamic H.264/H.265 decoder selection.
- Dynamic preview canvas sizing with aspect-ratio letterboxing.
- Colour-corruption fix: neutral (50) correction filters are skipped and slider mappings corrected.
- New circular app icon embedded in the exe (VCamdroid NEXT.exe).
- Version identity: VCamdroid NEXT, 2.0.0-NEXT.

### Android app
- Auto-connect immediately after a valid QR scan.
- Preview aspect ratio fixed (match_parent + AspectRatioMode.Adjust).
- TCP receive accumulator for split/coalesced packets.
- Keep screen awake while streaming.
- Neutral correction-filter values skipped (colour fix).
- New circular launcher icons and "VCamdroid NEXT" label.
- versionCode 200, versionName 2.0.0-NEXT.

### Packaging
- Installer: E:\Programs\Setup\VCamdroid\VCamdroid-NEXT-Setup.exe
- APK: E:\Programs\Setup\VCamdroid\VCamdroid-NEXT.apk
- Installed app: C:\Users\Admin\AppData\Local\Programs\VCamdroid
- Setup icon and app icon refreshed.

### Known / open items
- "Connecting and nothing happens" after phone connects: auto-switch implemented but needs live phone verification.
- StreamConfigDlg and ImgAdjDlg still use native controls (not yet M3-ified).
- DirectShow output resolution change requires an app restart (noted in UI).
- H.265 end-to-end not verified.
- Installer uses admin privileges with per-user install dir (Inno warning).
- Android launcher icon renders too zoomed-in/cropped; needs a less tight icon mask.
- Desktop in-app page back button looks non-standard; replace with standard M3 back arrow icon.
- Rotate buttons are hand-drawn; switch to standard M3 rotate-left/rotate-right icons instead of custom drawing.

## Next steps (resume here)

1. Deploy and test the new build on the phone:
   - Install VCamdroid-NEXT.apk (2.0.0-NEXT, versionCode 200).
   - Run VCamdroid NEXT desktop client.
   - Scan QR; verify auto-connect, preview aspect ratio, colours, keep-screen-on.
2. If the stream still does not start after connecting, investigate Manager::Connect2Stream and the RTSP receiver WorkerFunc on the PC side (receiver.cpp).
3. Convert StreamConfigDlg and ImgAdjDlg to Material 3 pages or dialogs (remove native controls).
4. Fix any remaining visual artifacts from user feedback (dropdown border, rotate arrows, icon cache).
5. Update the GitHub issue tracker (#1 and #2) with verification results.
6. Finalize README if needed, then commit and push to the fork.
7. Test the installer on a clean machine / MDM deployment.

## Notes
- Build commands:
  - Windows: MSBuild windows\VCamdroid.sln /m /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:FFMPEG_ROOT=...
  - Android: cd android; gradlew.bat assembleRelease
- Key paths:
  - Source: C:\Users\Admin\AppData\Local\Temp\opencode\vcamdroid-src
  - Installed: C:\Users\Admin\AppData\Local\Programs\VCamdroid
  - Setup: E:\Programs\Setup\VCamdroid
  - Fork: https://github.com/YossefKhalil/VCamdroid-NEXT
