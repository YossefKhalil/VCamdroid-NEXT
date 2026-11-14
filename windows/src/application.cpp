#include "application.h"

#include "gui/imgadjdlg.h"
#include "gui/streamconfigdlg.h"
#include "gui/qrconview.h"
#include "gui/theme.h"
#include "settings.h"
#include "video/guipreviewscaler.h"
#include <algorithm>
#include <iomanip>

Application::Application()
{
	wxInitAllImageHandlers();

	SetAppName("VCamdroid");
	
	Settings::Load();
	stateRegistry = Settings::GetDeviceStates();

	// Apply the saved theme (0 = system, 1 = light, 2 = dark) so native
	// controls follow the same scheme as the Material 3 surfaces.
	SetAppearance(Material::IsDark() ? Appearance::Dark : Appearance::Light);
}

bool Application::OnInit()
{
	if (!wxApp::OnInit())
		return false;

	switch (Settings::Get("DIRECTSHOW_RESOLUTION") + Window::MenuIDs::DS_SD)
	{
		case Window::MenuIDs::DS_SD: dsSource = std::make_unique<DirectShowSource>(640, 480); break;
		case Window::MenuIDs::DS_HD: dsSource = std::make_unique<DirectShowSource>(1280, 720); break;
		case Window::MenuIDs::DS_FHD: dsSource = std::make_unique<DirectShowSource>(1920, 1080); break;
		case Window::MenuIDs::DS_QHD: dsSource = std::make_unique<DirectShowSource>(3840, 2160); break;
		default: dsSource = std::make_unique<DirectShowSource>(1280, 720);
	}

	try {
		server = std::make_unique<Server>(6969, *this);
	}
	catch (const std::exception& e) {
		wxMessageBox(
			wxString::Format("Critical Error: Could not start network server.\n\nReason: %s\n\nPlease check if port 6969 is in use.", e.what()),
			"Initialization Failed",
			wxOK | wxICON_ERROR | wxCENTRE,
			nullptr
		);

		// Exit application if the server fails to start
		return false;
	}

	mainWindow = new Window(server->GetHostInfo());

	rtspManager = std::make_unique<RTSP::Manager>(
		*server,
		// OnFrameReceivedCallback
		[&](AVFrame* frame) {
			if (mainWindow && mainWindow->GetCanvas()) {
				mainWindow->GetCanvas()->ProcessRawFrameAsync(frame);
			}
			if (dsSource) {
				dsSource->SendRawFrame(frame);
			}
			snapshotManager.ProcessFrame(frame);

			// First decoded frame after a connection: flip the status chip.
			if (!streamLive.exchange(true) && mainWindow) {
				mainWindow->GetEventHandler()->CallAfter([this]() {
					if (mainWindow)
						mainWindow->SetStreaming(true);
				});
			}
		},
		// OnStatsReceivedCallback
		[&](const RTSP::Receiver::Stats& stats) {
			if (Settings::Get("SHOW_STATS") != 0 && mainWindow)
			{
				std::stringstream ss;
				// Bitrate with 1 decimal precision
				ss << stats.width << "p@" << (int)std::round(stats.fps) << "fps\n" << std::fixed << std::setprecision(1) << stats.bitrate << "Mbps";

				// Safe UI update
				mainWindow->GetEventHandler()->CallAfter([this, labelText = ss.str()]() {
					if (mainWindow && mainWindow->GetStatsText()) {
						mainWindow->GetStatsText()->SetLabelText(labelText);
						// Force layout to prevent overlap if text grows
						mainWindow->GetStatsText()->GetParent()->Layout();
					}
				});
			}
		}
	);

	BindEventListeners();
	appliedThemeMode = Settings::Get("THEME_MODE");
	mainWindow->Show();

	try {
		server->Start();
	}
	catch (const std::exception& e) {
		wxMessageBox(
			wxString::Format("Critical Error: Could not start the network listener.\n\nReason: %s", e.what()),
			"Initialization Failed",
			wxOK | wxICON_ERROR | wxCENTRE,
			mainWindow
		);
		return false;
	}

	return true;
}

StreamOptions& Application::GetCurrentDeviceStreamOptions()
{
	auto deviceId = rtspManager->GetStreamingDevice();
	// Safety check: if no device is streaming, return a dummy or handle error
	// For now assuming caller checks availability, but using .at() checks bounds
	const auto& desc = rtspManager->GetDescriptors().at(deviceId);
	return stateRegistry[desc.name()];
}

void Application::BindEventListeners()
{
	mainWindow->SetSettingsChangedCallback([this]() { ApplySettings(); });
	mainWindow->Bind(wxEVT_CLOSE_WINDOW, &Application::OnWindowCloseEvent, this);
	mainWindow->Bind(wxEVT_MENU, &Application::OnMenuEvent, this);

	mainWindow->GetSourceChoice()->Bind(wxEVT_CHOICE, &Application::OnSourceChanged, this);
	mainWindow->GetAdjustmentsButton()->Bind(wxEVT_BUTTON, &Application::ShowAdjustmentsDialog, this);
	mainWindow->GetStreamOptionsButton()->Bind(wxEVT_BUTTON, &Application::ShowStreamConfigDialog, this);

	mainWindow->GetRotateLeftButton()->Bind(wxEVT_BUTTON, [&](const wxEvent& arg) {
		rtspManager->Rotate(-90);
	});

	mainWindow->GetRotateRightButton()->Bind(wxEVT_BUTTON, [&](const wxEvent& arg) {
		rtspManager->Rotate(90);
	});

	mainWindow->GetFlipButton()->Bind(wxEVT_BUTTON, [&](const wxEvent& arg) {
		rtspManager->FlipHorizontally();
	});

	mainWindow->GetFlipVerticalButton()->Bind(wxEVT_BUTTON, [&](const wxEvent& arg) {
		rtspManager->FlipVertically();
	});

	// Check if devices exist before accessing options to prevent crashes
	mainWindow->GetTorchButton()->Bind(wxEVT_BUTTON, [&](const wxEvent& arg) {
		if (rtspManager->GetStreamingDevice() < 0) return;
		auto& options = GetCurrentDeviceStreamOptions();
		auto flash = options.flashEnabled = !options.flashEnabled;
		rtspManager->SetFlash(flash);
	});

	mainWindow->GetSwapButton()->Bind(wxEVT_BUTTON, [&](const wxEvent& arg) {
		if (rtspManager->GetStreamingDevice() < 0) return;
		auto& options = GetCurrentDeviceStreamOptions();
		options.backCameraActive = !options.backCameraActive;
		rtspManager->SwapCamera();
	});

	mainWindow->GetZoomInButton()->Bind(wxEVT_BUTTON, [&](const wxEvent& arg) {
		if (rtspManager->GetStreamingDevice() < 0) return;
		auto& options = GetCurrentDeviceStreamOptions();
		options.zoom = std::min(10.0f, options.zoom + 0.5f);
		mainWindow->GetZoomLevelLabel()->SetLabelText(wxString::Format("%.1fx", options.zoom));
		rtspManager->Zoom(options.zoom);
	});

	mainWindow->GetZoomOutButton()->Bind(wxEVT_BUTTON, [&](const wxEvent& arg) {
		if (rtspManager->GetStreamingDevice() < 0) return;
		auto& options = GetCurrentDeviceStreamOptions();
		options.zoom = std::max(1.0f, options.zoom - 0.5f);
		mainWindow->GetZoomLevelLabel()->SetLabelText(wxString::Format("%.1fx", options.zoom));
		rtspManager->Zoom(options.zoom);
	});

	mainWindow->GetSnapshotButton()->Bind(wxEVT_BUTTON, [&](const wxEvent& arg) {
		if (rtspManager->GetStreamingDevice() < 0) return;
		snapshotManager.RequestSnapshot();
	});
}

void Application::OnDeviceConnected(DeviceDescriptor& descriptor) const
{
	// Connection callbacks run on the asio I/O thread. Touching wxWidgets from
	// that thread is the root cause of the crash-on-connect reports, so marshal
	// everything onto the main thread and capture the descriptor by value.
	DeviceDescriptor copy = descriptor;
	mainWindow->GetEventHandler()->CallAfter([this, copy]() mutable {
		if (!mainWindow)
			return;
		mainWindow->GetTaskbarIcon()->ShowBalloon("New stream available",
			"Streaming device " + copy.name() + " available!", 10, wxICON_INFORMATION);
		rtspManager->AddDescriptor(copy);
		UpdateAvailableDevices();
		mainWindow->SetStatus(copy.name(), Material::Colors().primary);

		// Auto-switch to the freshly connected device so pairing a phone starts
		// the video immediately. StartStreamForDevice also moves the UI to the
		// preview page, so the QR code is not left on screen while connecting.
		if (!rtspManager->GetDescriptors().empty())
		{
			const int newId = static_cast<int>(rtspManager->GetDescriptors().size()) - 1;
			const_cast<Application*>(this)->StartStreamForDevice(newId);
		}
	});
}

void Application::OnDeviceDisconnected(DeviceDescriptor& descriptor) const
{
	DeviceDescriptor copy = descriptor;
	mainWindow->GetEventHandler()->CallAfter([this, copy]() mutable {
		if (!mainWindow)
			return;

		mainWindow->GetTaskbarIcon()->ShowBalloon("Stream ended",
			"Streaming device " + copy.name() + " disconnected!", 10, wxICON_INFORMATION);

		// Remember whether the device that just disconnected was the one we
		// were receiving from. RemoveDescriptor stops the RTSP receiver and
		// repairs the descriptor indices for us.
		const int streamingId = rtspManager->GetStreamingDevice();
		const bool wasStreaming =
			streamingId >= 0 &&
			streamingId < static_cast<int>(rtspManager->GetDescriptors().size()) &&
			rtspManager->GetDescriptors()[streamingId] == copy;

		rtspManager->RemoveDescriptor(copy);

		if (wasStreaming)
		{
			streamLive = false;
			if (mainWindow->GetCanvas())
				mainWindow->GetCanvas()->Clear();
			mainWindow->SetStreaming(false);
		}

		// Update UI list
		UpdateAvailableDevices();
		mainWindow->SetStatus("Waiting", Material::Colors().outline);
	});
}

void Application::OnDeviceErrorReported(DeviceDescriptor& descriptor, const Connection::ErrorReport& error) const
{
	const std::string deviceName = descriptor.name();
	Connection::ErrorReport copy = error;
	mainWindow->GetEventHandler()->CallAfter([this, deviceName, copy]() {
		if (!mainWindow)
			return;
		auto icon = copy.severity == Connection::ErrorReport::SEVERITY_WARNING ? wxICON_WARNING : wxICON_ERROR;
		mainWindow->GetTaskbarIcon()->ShowBalloon(deviceName + " " + copy.error, copy.description, 1000, icon);
	});
}

void Application::UpdateAvailableDevices() const
{
	auto choice = mainWindow->GetSourceChoice();
	int currentSelectionIndex = rtspManager->GetStreamingDevice();

	choice->Clear();

	const auto& devices = rtspManager->GetDescriptors();

	if (devices.empty())
	{
		// Logic Change: If empty, add placeholder and select it
		choice->Append("No devices");
		choice->SetSelection(0);

		// Optional: Clear stats since nothing is playing
		if (mainWindow->GetStatsText())
			mainWindow->GetStatsText()->SetLabelText("----p@--fps\n00.0Mbps");
	}
	else
	{
		choice->Append("Select device");

		// Repopulate
		for (auto& desc : devices)
			choice->Append(desc.name());

		if (currentSelectionIndex >= 0 && currentSelectionIndex < (int)devices.size())
		{
			// Only restore selection if valid
			choice->SetSelection(currentSelectionIndex + 1);
		}
		else
		{
			// If the previously selected device is gone, or we stopped, select nothing
			choice->SetSelection(0);
		}
	}
}

void Application::OnMenuEvent(wxCommandEvent& event)
{
	switch (event.GetId())
	{
		case Window::MenuIDs::DEVICES:
		{
			mainWindow->ShowDevicesPage(rtspManager->GetDescriptors());
			break;
		}

		case Window::MenuIDs::QR:
		case Window::MenuIDs::SHOW_QR:
		{
			mainWindow->ShowQrPage();
			break;
		}

		case Window::MenuIDs::STREAM_SETTINGS:
		{
			ShowStreamConfigDialog(event);
			break;
		}

		case Window::MenuIDs::SETTINGS:
		{
			mainWindow->ShowSettingsPage();
			break;
		}

		case Window::MenuIDs::HIDE2TRAY:
		{
			Settings::Set("MINIMIZE_TASKBAR", event.GetInt() != 0 ? 1 : 0);
			break;
		}

		case Window::MenuIDs::SHOWSTATS:
		{
			const bool show = event.GetInt() != 0;
			Settings::Set("SHOW_STATS", show ? 1 : 0);
			mainWindow->GetStatsText()->Show(show);
			mainWindow->Layout(); // Refresh layout to hide/show properly
			break;
		}

		case Window::MenuIDs::SAVESTATE:
		{
			Settings::Set("SAVE_DEVICE_STATES", event.GetInt() != 0 ? 1 : 0);
			break;
		}

		case Window::MenuIDs::THEME_SYSTEM:
			SetThemeMode(0);
			break;

		case Window::MenuIDs::THEME_LIGHT:
			SetThemeMode(1);
			break;

		case Window::MenuIDs::THEME_DARK:
			SetThemeMode(2);
			break;

		case Window::MenuIDs::ABOUT:
		{
			wxMessageBox(
				"VCamdroid NEXT\n\nWireless camera bridge that turns an Android phone into a webcam for Windows.\n\nMaterial 3 desktop client.",
				"About VCamdroid NEXT",
				wxOK | wxICON_INFORMATION,
				mainWindow);
			break;
		}

		case Window::MenuIDs::EXIT:
		{
			if (mainWindow)
				mainWindow->Close(true);
			break;
		}

		case Window::MenuIDs::DS_SD:
		case Window::MenuIDs::DS_HD:
		case Window::MenuIDs::DS_FHD:
		case Window::MenuIDs::DS_QHD:
		{
			Settings::Set("DIRECTSHOW_RESOLUTION", event.GetId() - Window::MenuIDs::DS_SD);
			break;
		}
	}
}

void Application::StartStreamForDevice(int deviceId)
{
	const auto& descriptors = rtspManager->GetDescriptors();
	if (deviceId < 0 || deviceId >= static_cast<int>(descriptors.size()))
		return;

	const auto& descriptor = descriptors[deviceId];

	EnsureStateInitialized(descriptor.name(), descriptor);
	auto& state = stateRegistry[descriptor.name()];

	// Validate the cached resolution against the current camera capabilities.
	const auto& resList = state.backCameraActive
		? descriptor.backResolutions()
		: descriptor.frontResolutions();

	if (resList.empty())
	{
		mainWindow->GetTaskbarIcon()->ShowBalloon("Error",
			"Device reported no supported resolutions.", 10, wxICON_WARNING);
		return;
	}

	if (std::find(resList.begin(), resList.end(), state.resolution) == resList.end())
		state.resolution = resList.front();

	// Keep the source selector in sync with the device that is streaming.
	if (mainWindow->GetSourceChoice()->GetSelection() != deviceId + 1)
		mainWindow->GetSourceChoice()->SetSelection(deviceId + 1);

	rtspManager->Connect2Stream(deviceId, state);

	streamLive = false;
	mainWindow->ShowPreviewPage();
	mainWindow->SetStatus("Connecting", Material::Colors().tertiary);
	mainWindow->GetZoomLevelLabel()->SetLabelText(wxString::Format("%.1fx", state.zoom));
}

void Application::StopStreaming()
{
	if (rtspManager)
		rtspManager->Disconnect();

	streamLive = false;
	if (mainWindow)
	{
		if (mainWindow->GetCanvas())
			mainWindow->GetCanvas()->Clear();
		mainWindow->SetStreaming(false);
	}
}

void Application::OnSourceChanged(wxEvent& event)
{
	// Index 0 is the "Select device" placeholder.
	const int deviceId = mainWindow->GetSourceChoice()->GetSelection() - 1;
	if (deviceId < 0)
		return;

	StartStreamForDevice(deviceId);
}

void Application::OnWindowCloseEvent(wxCloseEvent& event)
{
	// Hide window for responsive UI close feeling
	mainWindow->Hide();

	rtspManager.reset();
	server->Close();

	Settings::UpdateDeviceStates(stateRegistry);
	Settings::Save();

	event.Skip();
}

void Application::SetThemeMode(int mode)
{
	Settings::Set("THEME_MODE", mode);
	Settings::Save();

	if (mode == 1)
		SetAppearance(Appearance::Light);
	else if (mode == 2)
		SetAppearance(Appearance::Dark);
	else
		SetAppearance(Appearance::System);

	if (!mainWindow || !server)
		return;

	// Rebuild the frame so every Material surface resolves the new palette.
	// The RTSP receiver keeps running; the frame callback simply points at the
	// new canvas as soon as this returns.
	Window* old = mainWindow;
	mainWindow = nullptr;
	mainWindow = new Window(server->GetHostInfo());
	BindEventListeners();
	UpdateAvailableDevices();
	mainWindow->Show();

	if (old)
		old->Destroy();
}

void Application::ApplySettings()
{
	const int mode = Settings::Get("THEME_MODE");
	if (mode != appliedThemeMode)
	{
		appliedThemeMode = mode;
		// Rebuilding the frame from inside a control event would destroy the
		// control that is still handling the event, so defer it.
		if (mainWindow)
		{
			mainWindow->GetEventHandler()->CallAfter([this, mode]() {
				SetThemeMode(mode);
			});
		}
		return;
	}

	const bool show = Settings::Get("SHOW_STATS") == 1;
	if (mainWindow && mainWindow->GetStatsText())
	{
		mainWindow->GetStatsText()->Show(show);
		mainWindow->Layout();
	}
}

void Application::EnsureStateInitialized(std::string name, const DeviceDescriptor& descriptor)
{
	// operator[] creates the entry if it doesn't exist
	auto& state = stateRegistry[name];

	// Ensure defaults if this is a fresh entry
	if (state.zoom < 1.0f) state.zoom = 1.0f;

	// 1. Initialize Sliders (Default 50 if empty)
	if (descriptor.filters().count(Video::Filter::Category::CORRECTION))
	{
		for (const auto& fname : descriptor.filters().at(Video::Filter::Category::CORRECTION))
		{
			if (state.filterSliderValues.find(fname) == state.filterSliderValues.end())
			{
				state.filterSliderValues[fname] = 50;
			}
		}
	}
}

void Application::ShowAdjustmentsDialog(wxCommandEvent& event)
{
	if (rtspManager->GetDescriptors().empty()) return;

	int currentDeviceId = rtspManager->GetStreamingDevice();
	if (currentDeviceId < 0) return;

	const auto& desc = rtspManager->GetDescriptors()[currentDeviceId];

	EnsureStateInitialized(desc.name(), desc);
	auto& state = stateRegistry[desc.name()];

	ImgAdjDlg dialog(mainWindow, desc, state.filterSliderValues, state.activeEffectFilter);

	dialog.Bind(EVT_FILTER_PARAM_CHANGED, [&](const wxCommandEvent& event) 
	{
		auto name = event.GetString().ToStdString();
		auto value = event.GetInt();

		rtspManager->ApplyCorrectionFilter(name, value);
		state.filterSliderValues[name] = value;
	});

	dialog.Bind(EVT_FILTER_SWITCH_CHANGED, [&](const wxCommandEvent& event) 
	{
		auto name = event.GetString().ToStdString();
		// auto category = event.GetInt();

		rtspManager->ApplyEffectFilter(name);
		state.activeEffectFilter = name;
	});

	dialog.ShowModal();
}

void Application::ShowStreamConfigDialog(wxCommandEvent& event)
{
	int deviceId = rtspManager->GetStreamingDevice();
	if (deviceId < 0 || rtspManager->GetDescriptors().empty())
		return;

	const auto& desc = rtspManager->GetDescriptors()[deviceId];
	std::string deviceName = desc.name();

	EnsureStateInitialized(deviceName, desc);
	auto& state = stateRegistry[deviceName];

	StreamConfigDlg::Config config;

	config.resIndex = 0;
	const auto& resList = state.backCameraActive ? desc.backResolutions() : desc.frontResolutions();
	for (size_t i = 0; i < resList.size(); i++)
	{
		if (resList[i] == state.resolution)
		{
			config.resIndex = i;
			break;
		}
	}

	config.fps = state.fps;
	config.adaptiveBitrate = state.adaptiveBitrate;
	config.bitrate = state.bitrate;
	config.minBitrate = state.minBitrate;
	config.maxBitrate = state.maxBitrate;
	config.stabilizationEnabled = state.stabilizationEnabled;
	config.flashEnabled = state.flashEnabled;
	config.focusMode = state.focusMode;
	config.h265Enabled = state.h265Enabled;

	StreamConfigDlg dlg(mainWindow, desc, state.backCameraActive, config);

	dlg.Bind(EVT_STREAM_RESOLUTION_CHANGED, [this, deviceName](wxCommandEvent& e) 
	{
		wxString resStr = e.GetString(); // "1920 x 1080"
		long w = 0, h = 0;
		resStr.BeforeFirst('x').ToLong(&w);
		resStr.AfterFirst('x').ToLong(&h);

		rtspManager->SetResolution((uint16_t)w, (uint16_t)h);
		stateRegistry[deviceName].resolution = { (int)w, (int)h };
	});

	dlg.Bind(EVT_STREAM_FPS_CHANGED, [this, deviceName](wxCommandEvent& e) 
	{
		int fps = e.GetInt();
		rtspManager->SetFPS(fps);
		stateRegistry[deviceName].fps = fps;
	});

	dlg.Bind(EVT_STREAM_BITRATE_CHANGED, [this, deviceName, &dlg](wxCommandEvent& e) 
	{
		auto& regState = stateRegistry[deviceName];
		regState.adaptiveBitrate = dlg.IsAdaptiveBitrate();
		regState.bitrate = dlg.GetStaticBitrate();
		regState.minBitrate = dlg.GetMinBitrate();
		regState.maxBitrate = dlg.GetMaxBitrate();

		if (regState.adaptiveBitrate) 
		{
			rtspManager->SetAdaptiveBitrate(regState.minBitrate, regState.maxBitrate);
		}
		else 
		{
			rtspManager->SetBitrate(regState.bitrate);
		}
	});

	dlg.Bind(EVT_STREAM_CONFIG_CHANGED, [this, deviceName, &dlg](wxCommandEvent& e) 
	{
		auto& regState = stateRegistry[deviceName];

		if (regState.stabilizationEnabled != dlg.IsStabilizationEnabled()) 
		{
			regState.stabilizationEnabled = dlg.IsStabilizationEnabled();
			rtspManager->SetStabilization(regState.stabilizationEnabled);
		}

		if (regState.flashEnabled != dlg.IsFlashEnabled()) 
		{
			regState.flashEnabled = dlg.IsFlashEnabled();
			rtspManager->SetFlash(regState.flashEnabled);
		}

		if (regState.focusMode != dlg.GetFocusMode()) 
		{
			regState.focusMode = dlg.GetFocusMode();
			rtspManager->SetFocusMode(regState.focusMode);
		}

		if (regState.h265Enabled != dlg.IsH265Enabled()) 
		{
			regState.h265Enabled = dlg.IsH265Enabled();
			rtspManager->SetH265Codec(regState.h265Enabled);
		}
	});

	dlg.ShowModal();
}