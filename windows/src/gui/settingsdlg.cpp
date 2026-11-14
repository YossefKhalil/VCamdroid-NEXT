#include "gui/settingsdlg.h"
#include "gui/theme.h"
#include "settings.h"

SettingsDialog::SettingsDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	SetBackgroundColour(Material::Colors().background);

	auto mainSizer = new wxBoxSizer(wxVERTICAL);

	auto title = Material::Label(this, "Settings", Material::TypeRole::HeadlineSmall);
	mainSizer->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(24));
	mainSizer->AddSpacer(FromDIP(16));

	// Appearance
	auto appearance = new Material::Section(this, "Appearance");
	appearance->SetBackdrop(Material::Colors().background);
	themeSystem = new wxRadioButton(appearance, wxID_ANY, "Follow system", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	themeLight = new wxRadioButton(appearance, wxID_ANY, "Light");
	themeDark = new wxRadioButton(appearance, wxID_ANY, "Dark");
	Material::StyleRadioButton(themeSystem);
	Material::StyleRadioButton(themeLight);
	Material::StyleRadioButton(themeDark);
	const int themeMode = Settings::Get("THEME_MODE");
	themeLight->SetValue(themeMode == 1);
	themeDark->SetValue(themeMode == 2);
	themeSystem->SetValue(themeMode != 1 && themeMode != 2);
	appearance->Body()->Add(themeSystem, 0, wxBOTTOM, FromDIP(6));
	appearance->Body()->Add(themeLight, 0, wxBOTTOM, FromDIP(6));
	appearance->Body()->Add(themeDark, 0);
	mainSizer->Add(appearance, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
	mainSizer->AddSpacer(FromDIP(16));

	// Virtual camera
	auto camera = new Material::Section(this, "Virtual camera",
		"DirectShow output resolution. Requires an app restart.");
	camera->SetBackdrop(Material::Colors().background);
	dsSd = new wxRadioButton(camera, wxID_ANY, "640 x 480", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	dsHd = new wxRadioButton(camera, wxID_ANY, "1280 x 720");
	dsFhd = new wxRadioButton(camera, wxID_ANY, "1920 x 1080");
	dsQhd = new wxRadioButton(camera, wxID_ANY, "3840 x 2160");
	Material::StyleRadioButton(dsSd);
	Material::StyleRadioButton(dsHd);
	Material::StyleRadioButton(dsFhd);
	Material::StyleRadioButton(dsQhd);
	const int dsRes = Settings::Get("DIRECTSHOW_RESOLUTION");
	dsSd->SetValue(dsRes <= 0);
	dsHd->SetValue(dsRes == 1);
	dsFhd->SetValue(dsRes == 2);
	dsQhd->SetValue(dsRes == 3);
	camera->Body()->Add(dsSd, 0, wxBOTTOM, FromDIP(6));
	camera->Body()->Add(dsHd, 0, wxBOTTOM, FromDIP(6));
	camera->Body()->Add(dsFhd, 0, wxBOTTOM, FromDIP(6));
	camera->Body()->Add(dsQhd, 0);
	mainSizer->Add(camera, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
	mainSizer->AddSpacer(FromDIP(16));

	// Behavior
	auto behavior = new Material::Section(this, "Behavior");
	behavior->SetBackdrop(Material::Colors().background);
	showStats = new wxCheckBox(behavior, wxID_ANY, "Show frame stats");
	hideToTray = new wxCheckBox(behavior, wxID_ANY, "Hide to tray when minimised");
	savePresets = new wxCheckBox(behavior, wxID_ANY, "Save device presets");
	Material::StyleCheckBox(showStats);
	Material::StyleCheckBox(hideToTray);
	Material::StyleCheckBox(savePresets);
	showStats->SetValue(Settings::Get("SHOW_STATS") == 1);
	hideToTray->SetValue(Settings::Get("MINIMIZE_TASKBAR") == 1);
	savePresets->SetValue(Settings::Get("SAVE_DEVICE_STATES") == 1);
	behavior->Body()->Add(showStats, 0, wxBOTTOM, FromDIP(6));
	behavior->Body()->Add(hideToTray, 0, wxBOTTOM, FromDIP(6));
	behavior->Body()->Add(savePresets, 0);
	mainSizer->Add(behavior, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
	mainSizer->AddSpacer(FromDIP(20));

	auto buttonRow = new wxBoxSizer(wxHORIZONTAL);
	buttonRow->AddStretchSpacer(1);
	auto done = new Material::PillButton(this, "Done", Material::PillButton::Variant::Filled);
	done->SetBackdrop(Material::Colors().background);
	done->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CallAfter([this]() { EndModal(wxID_OK); }); });
	buttonRow->Add(done, 0);
	mainSizer->Add(buttonRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(24));

	SetSizerAndFit(mainSizer);
	SetMinSize(FromDIP(wxSize(480, 560)));
	CenterOnParent();
}

int SettingsDialog::GetThemeMode() const
{
	if (themeLight->GetValue()) return 1;
	if (themeDark->GetValue()) return 2;
	return 0;
}

int SettingsDialog::GetDirectShowResolution() const
{
	if (dsHd->GetValue()) return 1;
	if (dsFhd->GetValue()) return 2;
	if (dsQhd->GetValue()) return 3;
	return 0;
}

bool SettingsDialog::GetShowStats() const { return showStats->GetValue(); }
bool SettingsDialog::GetHideToTray() const { return hideToTray->GetValue(); }
bool SettingsDialog::GetSavePresets() const { return savePresets->GetValue(); }
