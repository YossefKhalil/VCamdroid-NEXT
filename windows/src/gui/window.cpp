#include "gui/window.h"
#include "gui/settingspanel.h"
#include "gui/devicespanel.h"
#include "settings.h"
#include "icon.xpm"

#include <wx/statline.h>
#include <wx/simplebook.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <cstring>

#include "qrcodegen.hpp"

namespace
{
	// QR code with the quiet zone required by the specification, rendered with
	// nearest-neighbour scaling so the modules stay crisp.
	wxBitmap MakeQrBitmap(const wxString& data, int displaySize)
	{
		const qrcodegen::QrCode qrcode = qrcodegen::QrCode::encodeText(
			data.utf8_str().data(), qrcodegen::QrCode::Ecc::MEDIUM);
		const int size = qrcode.getSize();
		const int border = 8;
		const int total = size + border * 2;

		wxImage image(total, total);
		unsigned char* bytes = image.GetData();
		std::memset(bytes, 255, static_cast<size_t>(total) * total * 3);

		for (int y = 0; y < size; y++)
		{
			for (int x = 0; x < size; x++)
			{
				if (qrcode.getModule(x, y))
				{
					unsigned char* pixel = bytes + (static_cast<size_t>(y + border) * total + (x + border)) * 3;
					pixel[0] = 0;
					pixel[1] = 0;
					pixel[2] = 0;
				}
			}
		}

		return wxBitmap(image.Scale(wxSize(displaySize, displaySize), wxIMAGE_QUALITY_NEAREST));
	}
}

Window::Window(Server::HostInfo hostinfo)
	: wxFrame(nullptr, wxID_ANY, "VCamdroid NEXT", wxDefaultPosition, wxSize(880, 680), wxDEFAULT_FRAME_STYLE)
{
	SetBackgroundColour(Material::Colors().background);
	SetMinSize(FromDIP(wxSize(720, 560)));

	wxFileName exeFile(wxStandardPaths::Get().GetExecutablePath());
	appDir = exeFile.GetPath();

	appIcon = wxIcon(icon);
	wxIcon fileIcon;
	const wxString iconPath = appDir + wxFILE_SEP_PATH + "res" + wxFILE_SEP_PATH + "vcamdroid.ico";
	if (fileIcon.LoadFile(iconPath, wxBITMAP_TYPE_ICO))
		appIcon = fileIcon;

	taskbarIcon = new wxTaskBarIcon();
	taskbarIcon->SetIcon(appIcon, "VCamdroid NEXT");
	taskbarIcon->Bind(wxEVT_TASKBAR_LEFT_DCLICK, &Window::MaximizeFromTaskbar, this);
	taskbarIcon->Bind(wxEVT_TASKBAR_RIGHT_UP, [this](wxTaskBarIconEvent&) { ShowTaskbarMenu(); });
	Bind(wxEVT_ICONIZE, &Window::MinimizeToTaskbar, this);

	SetIcon(appIcon);

	auto root = new wxPanel(this, wxID_ANY);
	root->SetBackgroundColour(Material::Colors().background);

	auto topsizer = new wxBoxSizer(wxVERTICAL);
	InitializeHeader(root, topsizer);
	InitializeSourceBar(root, topsizer);
	InitializeContent(root, topsizer, hostinfo);
	InitializeToolbar(root, topsizer);

	root->SetSizer(topsizer);
	Center();
}

Window::~Window()
{
	delete taskbarIcon;
}

void Window::InitializeHeader(wxPanel* parent, wxBoxSizer* topsizer)
{
	auto header = new wxPanel(parent, wxID_ANY);
	header->SetBackgroundColour(Material::Colors().background);

	auto row = new wxBoxSizer(wxHORIZONTAL);

	backButton = new Material::IconButton(header, Material::Icon::ArrowBack, "Back");
	backButton->SetBackdrop(header->GetBackgroundColour());
	backButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { GoBack(); });
	backButton->Hide();
	row->Add(backButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));

	wxBitmap logoBitmap;
	wxImage logoImage;
	const wxString logoPath = appDir + wxFILE_SEP_PATH + "res" + wxFILE_SEP_PATH + "logo.png";
	if (logoImage.LoadFile(logoPath, wxBITMAP_TYPE_PNG) && logoImage.IsOk())
	{
		logoBitmap = wxBitmap(logoImage.Scale(FromDIP(36), FromDIP(36), wxIMAGE_QUALITY_HIGH));
	}
	else
	{
		logoBitmap = wxBitmap(icon);
		if (logoBitmap.IsOk())
			wxBitmap::Rescale(logoBitmap, wxSize(FromDIP(36), FromDIP(36)));
	}
	auto logo = new wxStaticBitmap(header, wxID_ANY, logoBitmap);
	row->Add(logo, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));

	titleLabel = Material::Label(header, "VCamdroid NEXT", Material::TypeRole::TitleLarge);
	row->Add(titleLabel, 0, wxALIGN_CENTER_VERTICAL);

	row->AddStretchSpacer(1);

	statusChip = new Material::Chip(header, "Waiting", Material::Colors().outline, false);
	statusChip->SetTonal(Material::Colors().surfaceContainerHigh,
		Material::Colors().onSurfaceVariant, Material::Colors().outline);
	statusChip->SetBackgroundColour(header->GetBackgroundColour());
	row->Add(statusChip, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));

	qrButton = new Material::IconButton(header, Material::Icon::QrCode, "Show QR code");
	qrButton->SetBackdrop(header->GetBackgroundColour());
	qrButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowQrPage(); });
	row->Add(qrButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));

	menuButton = new Material::IconButton(header, Material::Icon::MoreVert, "Menu");
	menuButton->SetBackdrop(header->GetBackgroundColour());
	menuButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowMainMenu(); });
	row->Add(menuButton, 0, wxALIGN_CENTER_VERTICAL);

	header->SetSizer(row);
	topsizer->Add(header, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(20));
}

void Window::InitializeSourceBar(wxPanel* parent, wxBoxSizer* topsizer)
{
	auto card = new Material::Card(parent, 12, Material::Shape::Medium);
	card->SetBackdrop(Material::Colors().background);
	sourceCard = card;

	auto row = new wxBoxSizer(wxHORIZONTAL);

	sourceChoice = new Material::Select(card, "No devices");
	sourceChoice->Append("No devices");
	sourceChoice->SetSelection(0);
	sourceChoice->SetMinSize(wxSize(FromDIP(280), FromDIP(40)));
	row->Add(sourceChoice, 0, wxALIGN_CENTER_VERTICAL);

	row->AddStretchSpacer(1);

	statsText = new wxStaticText(card, wxID_ANY, "----p @ -- fps\n0.0 Mbps",
		wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	statsText->SetFont(Material::Font(Material::TypeRole::BodySmall));
	statsText->SetForegroundColour(Material::Colors().onSurfaceVariant);
	statsText->SetBackgroundColour(card->GetBackgroundColour());
	statsText->Show(Settings::Get("SHOW_STATS") == 1);
	row->Add(statsText, 0, wxALIGN_CENTER_VERTICAL);

	card->Content()->Add(row, 1, wxEXPAND);
	topsizer->Add(card, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(20));
}

void Window::InitializeContent(wxPanel* parent, wxBoxSizer* topsizer, Server::HostInfo hostinfo)
{
	book = new wxSimplebook(parent, wxID_ANY);
	book->SetBackgroundColour(Material::Colors().background);

	// Page 0: the QR code is always ready to scan, so connecting does not
	// require opening a dialog first.
	auto qrPage = new wxPanel(book, wxID_ANY);
	qrPage->SetBackgroundColour(Material::Colors().background);
	auto qrSizer = new wxBoxSizer(wxVERTICAL);

	auto qrCard = new Material::Card(qrPage, 24, Material::Shape::Large, true);
	qrCard->SetBackdrop(Material::Colors().background);
	qrCard->SetFill(*wxWHITE);

	const wxString payload = wxString::Format("%s:%s", std::get<1>(hostinfo), std::get<2>(hostinfo));
	const int qrSize = FromDIP(260);
	auto qrImage = new wxStaticBitmap(qrCard, wxID_ANY, MakeQrBitmap(payload, qrSize));
	qrImage->SetBackgroundColour(*wxWHITE);
	qrCard->Content()->Add(qrImage, 0, wxALIGN_CENTER_HORIZONTAL);

	qrSizer->AddStretchSpacer(1);
	qrSizer->Add(qrCard, 0, wxALIGN_CENTER_HORIZONTAL);
	qrSizer->AddSpacer(FromDIP(12));
	auto caption = Material::Label(qrPage, "Scan with the VCamdroid Android app",
		Material::TypeRole::BodySmall, Material::Colors().onSurfaceVariant);
	qrSizer->Add(caption, 0, wxALIGN_CENTER_HORIZONTAL);
	qrSizer->AddStretchSpacer(1);
	qrPage->SetSizer(qrSizer);

	// Page 1: live preview.
	auto videoPage = new wxPanel(book, wxID_ANY);
	videoPage->SetBackgroundColour(Material::Colors().background);
	auto videoSizer = new wxBoxSizer(wxVERTICAL);

	auto videoCard = new Material::Card(videoPage, 8, Material::Shape::Large, true);
	videoCard->SetBackdrop(Material::Colors().background);
	videoCard->SetFill(Material::Colors().surfaceContainerHighest);

	canvas = new Canvas(videoCard, wxDefaultPosition, wxSize(720, 405));
	canvas->SetCornerRadius(Material::Radius(Material::Shape::Large));
	canvas->SetMinSize(wxSize(FromDIP(480), FromDIP(270)));
	canvas->SetPlaceholder("Starting camera", "");
	videoCard->Content()->Add(canvas, 1, wxEXPAND);

	videoSizer->Add(videoCard, 1, wxEXPAND);
	videoPage->SetSizer(videoSizer);

	// Page 2: settings (in-app page, no modal dialog).
	settingsPage = new wxPanel(book, wxID_ANY);
	settingsPage->SetBackgroundColour(Material::Colors().background);
	auto settingsSizer = new wxBoxSizer(wxVERTICAL);

	auto settingsScroll = new wxScrolledWindow(settingsPage, wxID_ANY);
	settingsScroll->SetBackgroundColour(Material::Colors().background);
	settingsScroll->SetScrollRate(0, FromDIP(12));
	settingsPanel = new SettingsPanel(settingsScroll, [this]() {
		if (settingsChanged)
			settingsChanged();
	});
	auto scrollSizer = new wxBoxSizer(wxVERTICAL);
	scrollSizer->Add(settingsPanel, 1, wxEXPAND);
	settingsScroll->SetSizer(scrollSizer);
	settingsScroll->FitInside();

	settingsSizer->Add(settingsScroll, 1, wxEXPAND);
	settingsPage->SetSizer(settingsSizer);

	// Page 3: connected devices (in-app page, no modal dialog).
	devicesPage = new wxPanel(book, wxID_ANY);
	devicesPage->SetBackgroundColour(Material::Colors().background);
	auto devicesSizer = new wxBoxSizer(wxVERTICAL);
	devicesPanel = new DevicesPanel(devicesPage);
	devicesSizer->Add(devicesPanel, 1, wxEXPAND);
	devicesPage->SetSizer(devicesSizer);

	book->AddPage(qrPage, "QR");
	book->AddPage(videoPage, "Preview");
	book->AddPage(settingsPage, "Settings");
	book->AddPage(devicesPage, "Devices");
	book->SetSelection(0);

	topsizer->Add(book, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(20));
}

void Window::InitializeToolbar(wxPanel* parent, wxBoxSizer* topsizer)
{
	auto toolbar = new Material::Card(parent, 8, Material::Shape::Large);
	toolbar->SetBackdrop(Material::Colors().background);
	toolbarCard = toolbar;

	auto row = new wxBoxSizer(wxHORIZONTAL);

	auto addIcon = [&](Material::Icon icon, const wxString& tip, Material::IconButton** out)
	{
		auto button = new Material::IconButton(toolbar, icon, tip);
		button->SetBackdrop(toolbar->GetBackgroundColour());
		row->Add(button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(2));
		if (out)
			*out = button;
	};

	auto addSeparator = [&]()
	{
		auto separator = new wxPanel(toolbar, wxID_ANY, wxDefaultPosition,
			wxSize(FromDIP(1), FromDIP(26)));
		separator->SetBackgroundColour(Material::Colors().outlineVariant);
		row->Add(separator, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(8));
	};

	addIcon(Material::Icon::RotateLeft, "Rotate left", &rotateLeftButton);
	addIcon(Material::Icon::RotateRight, "Rotate right", &rotateRightButton);
	addIcon(Material::Icon::FlipHorizontal, "Flip horizontally", &flipButton);
	addIcon(Material::Icon::FlipVertical, "Flip vertically", &flipVerticalButton);

	addSeparator();

	addIcon(Material::Icon::ZoomOut, "Zoom out", &zoomOutButton);

	zoomLevelLabel = new Material::Chip(toolbar, "1.0x", wxNullColour, false);
	row->Add(zoomLevelLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(6));

	addIcon(Material::Icon::ZoomIn, "Zoom in", &zoomInButton);

	addSeparator();

	addIcon(Material::Icon::Flash, "Toggle flash", &torchButton);
	addIcon(Material::Icon::SwapCamera, "Switch camera", &swapButton);
	addIcon(Material::Icon::Snapshot, "Take snapshot", &snapshotButton);

	row->AddStretchSpacer(1);

	addIcon(Material::Icon::Adjustments, "Image adjustments", &adjustmentsButton);
	addIcon(Material::Icon::Gear, "Stream settings", &streamOptionsButton);

	toolbar->Content()->Add(row, 1, wxEXPAND);
	topsizer->Add(toolbar, 0, wxEXPAND | wxALL, FromDIP(20));
}

void Window::ShowMainMenu()
{
	using Material::MenuItem;

	std::vector<MenuItem> items;
	items.push_back(MenuItem::Item(MenuIDs::DEVICES, "Connected devices"));
	items.push_back(MenuItem::Item(MenuIDs::SETTINGS, "Settings"));
	items.push_back(MenuItem::Separator());
	items.push_back(MenuItem::Item(MenuIDs::ABOUT, "About"));
	items.push_back(MenuItem::Item(MenuIDs::EXIT, "Exit"));

	const wxPoint anchor = menuButton ? menuButton->ClientToScreen(wxPoint(0, menuButton->GetSize().y)) : wxGetMousePosition();
	auto* menu = new Material::MenuPopup(this, items, [this](int id, bool checked)
	{
		SendMenuEvent(id, checked);
	});
	const wxSize popupSize = menu->GetPopupSize();
	const int popupX = menuButton ? (anchor.x + menuButton->GetSize().x - popupSize.x) : anchor.x;
	menu->PopupAt(wxPoint(popupX, anchor.y));
}

void Window::SendMenuEvent(int id, bool checked)
{
	wxCommandEvent event(wxEVT_MENU, id);
	event.SetEventObject(this);
	event.SetInt(checked ? 1 : 0);
	GetEventHandler()->ProcessEvent(event);
}

void Window::SetStatus(const wxString& text, const wxColour& dotColor)
{
	if (!statusChip)
		return;
	statusChip->SetLabelText(text);
	statusChip->SetTonal(Material::Colors().surfaceContainerHigh,
		Material::Colors().onSurfaceVariant, dotColor);
	statusChip->GetParent()->Layout();
}

void Window::SetStreaming(bool live)
{
	SetStatus(live ? "Live" : "Waiting",
		live ? Material::Colors().primary : Material::Colors().outline);

	if (live && statusChip)
		statusChip->SetTonal(Material::Colors().primaryContainer,
			Material::Colors().onPrimaryContainer, Material::Colors().primary);

	contentPage = live ? 1 : 0;

	// Only switch the content page when the user is on a content page, so the
	// Settings/Devices pages are not yanked away during a stream change.
	if (book && book->GetSelection() <= 1)
		book->ChangeSelection(contentPage);

	backButton->Hide();
	qrButton->Show();
	sourceCard->Show();
	toolbarCard->Show();
	titleLabel->SetLabel("VCamdroid NEXT");
	backButton->GetParent()->Layout();
	Layout();
}

void Window::ShowQrPage()
{
	contentPage = 0;
	if (book)
		book->ChangeSelection(0);
	backButton->Hide();
	qrButton->Show();
	sourceCard->Show();
	toolbarCard->Show();
	titleLabel->SetLabel("VCamdroid NEXT");
	backButton->GetParent()->Layout();
	Layout();
}

// Switch to the preview as soon as a stream is requested, so a connected
// device never leaves the user staring at the pairing QR code while the
// RTSP connection is still being established.
void Window::ShowPreviewPage()
{
	contentPage = 1;
	if (book)
		book->ChangeSelection(1);
	backButton->Hide();
	qrButton->Show();
	sourceCard->Show();
	toolbarCard->Show();
	titleLabel->SetLabel("VCamdroid NEXT");
	backButton->GetParent()->Layout();
	Layout();
}

void Window::ShowSettingsPage()
{
	if (book)
		book->ChangeSelection(2);
	backButton->Show();
	qrButton->Hide();
	sourceCard->Hide();
	toolbarCard->Hide();
	titleLabel->SetLabel("Settings");
	backButton->GetParent()->Layout();
	Layout();
}

void Window::ShowDevicesPage(const std::vector<DeviceDescriptor>& devices)
{
	if (devicesPanel)
		devicesPanel->SetDevices(devices);
	if (book)
		book->ChangeSelection(3);
	backButton->Show();
	qrButton->Hide();
	sourceCard->Hide();
	toolbarCard->Hide();
	titleLabel->SetLabel("Connected devices");
	backButton->GetParent()->Layout();
	Layout();
}

void Window::GoBack()
{
	if (book)
		book->ChangeSelection(contentPage);
	backButton->Hide();
	qrButton->Show();
	sourceCard->Show();
	toolbarCard->Show();
	titleLabel->SetLabel("VCamdroid NEXT");
	backButton->GetParent()->Layout();
	Layout();
}

void Window::ShowTaskbarMenu()
{
	wxMenu menu;
	menu.Append(wxID_OPEN, "Open VCamdroid NEXT");
	menu.AppendSeparator();
	menu.Append(wxID_EXIT, "Exit");

	menu.Bind(wxEVT_MENU, [this](wxCommandEvent& event)
	{
		if (event.GetId() == wxID_OPEN)
		{
			Iconize(false);
			Show();
			Raise();
			SetFocus();
		}
		else if (event.GetId() == wxID_EXIT)
		{
			Close(true);
		}
	});

	if (taskbarIcon)
		taskbarIcon->PopupMenu(&menu);
}

void Window::MinimizeToTaskbar(wxIconizeEvent& evt)
{
	if (Settings::Get("MINIMIZE_TASKBAR") == 1)
	{
		this->Hide();
		evt.Skip();
	}
}

void Window::MaximizeFromTaskbar(wxTaskBarIconEvent& evt)
{
	this->Iconize(false);
	this->SetFocus();
	this->Raise();
	this->Show();
}

Canvas* Window::GetCanvas() { return canvas; }
Material::Select* Window::GetSourceChoice() { return sourceChoice; }
wxWindow* Window::GetStreamOptionsButton() { return streamOptionsButton; }

wxWindow* Window::GetRotateLeftButton() { return rotateLeftButton; }
wxWindow* Window::GetRotateRightButton() { return rotateRightButton; }
wxWindow* Window::GetFlipButton() { return flipButton; }
wxWindow* Window::GetFlipVerticalButton() { return flipVerticalButton; }

wxWindow* Window::GetZoomInButton() { return zoomInButton; }
wxWindow* Window::GetZoomOutButton() { return zoomOutButton; }
Material::Chip* Window::GetZoomLevelLabel() { return zoomLevelLabel; }

wxWindow* Window::GetTorchButton() { return torchButton; }
wxWindow* Window::GetSwapButton() { return swapButton; }

wxWindow* Window::GetAdjustmentsButton() { return adjustmentsButton; }
wxWindow* Window::GetSnapshotButton() { return snapshotButton; }

wxStaticText* Window::GetStatsText() { return statsText; }
wxTaskBarIcon* Window::GetTaskbarIcon() { return taskbarIcon; }
