#pragma once

#include <wx/wx.h>
#include <wx/taskbar.h>
#include <map>
#include <vector>
#include <functional>

#include "canvas.h"
#include "net/server.h"
#include "net/devicedescriptor.h"
#include "gui/theme.h"

class wxSimplebook;
class SettingsPanel;
class DevicesPanel;

class Window : public wxFrame
{
public:
	struct MenuIDs
	{
		static const int QR = 100;
		static const int DEVICES = 101;
		static const int HIDE2TRAY = 102;
		static const int SHOWSTATS = 103;
		static const int SAVESTATE = 104;
		static const int DS_SD = 105;
		static const int DS_HD = 106;
		static const int DS_FHD = 107;
		static const int DS_QHD = 108;
		static const int ABOUT = 109;
		static const int EXIT = 110;
		static const int THEME_SYSTEM = 111;
		static const int THEME_LIGHT = 112;
		static const int THEME_DARK = 113;
		static const int STREAM_SETTINGS = 114;
		static const int SHOW_QR = 115;
		static const int SETTINGS = 116;
	};

	Window(Server::HostInfo hostInfo);
	~Window();

	Canvas* GetCanvas();
	Material::Select* GetSourceChoice();
	wxWindow* GetStreamOptionsButton();

	// Bottom toolbar controls
	wxWindow* GetRotateLeftButton();
	wxWindow* GetRotateRightButton();
	wxWindow* GetFlipButton();
	wxWindow* GetFlipVerticalButton();

	wxWindow* GetZoomInButton();
	wxWindow* GetZoomOutButton();
	Material::Chip* GetZoomLevelLabel();

	wxWindow* GetTorchButton();
	wxWindow* GetSwapButton();

	wxWindow* GetAdjustmentsButton();
	wxWindow* GetSnapshotButton();

	wxStaticText* GetStatsText();
	wxTaskBarIcon* GetTaskbarIcon();

	// Material 3 status helpers.
	void SetStatus(const wxString& text, const wxColour& dotColor);
	void SetStreaming(bool live);
	void ShowQrPage();
	void ShowPreviewPage();

	void ShowSettingsPage();
	void ShowDevicesPage(const std::vector<DeviceDescriptor>& devices);
	void GoBack();
	void ShowTaskbarMenu();
	void SetSettingsChangedCallback(std::function<void()> callback) { settingsChanged = std::move(callback); }

private:
	wxTaskBarIcon* taskbarIcon = nullptr;
	wxIcon appIcon;
	wxString appDir;
	wxSimplebook* book = nullptr;
	Canvas* canvas = nullptr;
	wxPanel* settingsPage = nullptr;
	wxPanel* devicesPage = nullptr;
	SettingsPanel* settingsPanel = nullptr;
	DevicesPanel* devicesPanel = nullptr;
	int contentPage = 0;
	std::function<void()> settingsChanged;

	Material::Select* sourceChoice = nullptr;
	wxStaticText* statsText = nullptr;
	wxStaticText* titleLabel = nullptr;

	Material::Chip* statusChip = nullptr;
	Material::IconButton* menuButton = nullptr;
	Material::IconButton* qrButton = nullptr;
	Material::IconButton* backButton = nullptr;
	Material::IconButton* streamOptionsButton = nullptr;
	Material::Card* sourceCard = nullptr;
	Material::Card* toolbarCard = nullptr;

	Material::IconButton* rotateLeftButton = nullptr;
	Material::IconButton* rotateRightButton = nullptr;
	Material::IconButton* flipButton = nullptr;
	Material::IconButton* flipVerticalButton = nullptr;

	Material::IconButton* zoomInButton = nullptr;
	Material::IconButton* zoomOutButton = nullptr;
	Material::Chip* zoomLevelLabel = nullptr;

	Material::IconButton* torchButton = nullptr;
	Material::IconButton* swapButton = nullptr;

	Material::IconButton* adjustmentsButton = nullptr;
	Material::IconButton* snapshotButton = nullptr;

	void InitializeHeader(wxPanel* parent, wxBoxSizer* topsizer);
	void InitializeSourceBar(wxPanel* parent, wxBoxSizer* topsizer);
	void InitializeContent(wxPanel* parent, wxBoxSizer* topsizer, Server::HostInfo hostinfo);
	void InitializeToolbar(wxPanel* parent, wxBoxSizer* topsizer);

	void ShowMainMenu();
	void SendMenuEvent(int id, bool checked = false);

	void MinimizeToTaskbar(wxIconizeEvent& evt);
	void MaximizeFromTaskbar(wxTaskBarIconEvent& evt);
};
