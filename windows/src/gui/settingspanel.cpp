#include "gui/settingspanel.h"
#include "gui/theme.h"
#include "settings.h"

SettingsPanel::SettingsPanel(wxWindow* parent, std::function<void()> onChanged)
	: wxPanel(parent, wxID_ANY),
	onChanged(std::move(onChanged))
{
	SetBackgroundColour(Material::Colors().background);

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->AddSpacer(FromDIP(20));

	auto appearance = new Material::Section(this, "Appearance");
	appearance->SetBackdrop(Material::Colors().background);
	themeSystem = new Material::SettingsRow(appearance, "Follow system",
		"Match the Windows theme", Material::SettingsRow::Mode::Radio);
	themeLight = new Material::SettingsRow(appearance, "Light", "", Material::SettingsRow::Mode::Radio);
	themeDark = new Material::SettingsRow(appearance, "Dark", "", Material::SettingsRow::Mode::Radio);
	appearance->Body()->Add(themeSystem, 0, wxEXPAND | wxBOTTOM, FromDIP(2));
	appearance->Body()->Add(themeLight, 0, wxEXPAND | wxBOTTOM, FromDIP(2));
	appearance->Body()->Add(themeDark, 0, wxEXPAND);
	mainSizer->Add(appearance, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
	mainSizer->AddSpacer(FromDIP(16));

	auto camera = new Material::Section(this, "Virtual camera",
		"DirectShow output resolution. Requires an app restart.");
	camera->SetBackdrop(Material::Colors().background);
	dsSd = new Material::SettingsRow(camera, "640 x 480", "", Material::SettingsRow::Mode::Radio);
	dsHd = new Material::SettingsRow(camera, "1280 x 720", "", Material::SettingsRow::Mode::Radio);
	dsFhd = new Material::SettingsRow(camera, "1920 x 1080", "", Material::SettingsRow::Mode::Radio);
	dsQhd = new Material::SettingsRow(camera, "3840 x 2160", "", Material::SettingsRow::Mode::Radio);
	camera->Body()->Add(dsSd, 0, wxEXPAND | wxBOTTOM, FromDIP(2));
	camera->Body()->Add(dsHd, 0, wxEXPAND | wxBOTTOM, FromDIP(2));
	camera->Body()->Add(dsFhd, 0, wxEXPAND | wxBOTTOM, FromDIP(2));
	camera->Body()->Add(dsQhd, 0, wxEXPAND);
	mainSizer->Add(camera, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
	mainSizer->AddSpacer(FromDIP(16));

	auto behavior = new Material::Section(this, "Behavior");
	behavior->SetBackdrop(Material::Colors().background);
	showStats = new Material::SettingsRow(behavior, "Show frame stats",
		"Overlay resolution, FPS and bitrate", Material::SettingsRow::Mode::Check);
	hideToTray = new Material::SettingsRow(behavior, "Hide to tray when minimised", "",
		Material::SettingsRow::Mode::Check);
	savePresets = new Material::SettingsRow(behavior, "Save device presets",
		"Remember settings per device", Material::SettingsRow::Mode::Check);
	behavior->Body()->Add(showStats, 0, wxEXPAND | wxBOTTOM, FromDIP(2));
	behavior->Body()->Add(hideToTray, 0, wxEXPAND | wxBOTTOM, FromDIP(2));
	behavior->Body()->Add(savePresets, 0, wxEXPAND);
	mainSizer->Add(behavior, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
	mainSizer->AddSpacer(FromDIP(24));

	SetSizer(mainSizer);

	themeSystem->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { SelectTheme(themeSystem); });
	themeLight->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { SelectTheme(themeLight); });
	themeDark->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { SelectTheme(themeDark); });
	dsSd->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { SelectResolution(dsSd); });
	dsHd->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { SelectResolution(dsHd); });
	dsFhd->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { SelectResolution(dsFhd); });
	dsQhd->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { SelectResolution(dsQhd); });
	showStats->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { Commit(); });
	hideToTray->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { Commit(); });
	savePresets->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { Commit(); });

	Reload();
}

void SettingsPanel::Reload()
{
	const int themeMode = Settings::Get("THEME_MODE");
	themeLight->SetChecked(themeMode == 1);
	themeDark->SetChecked(themeMode == 2);
	themeSystem->SetChecked(themeMode != 1 && themeMode != 2);

	const int dsRes = Settings::Get("DIRECTSHOW_RESOLUTION");
	dsSd->SetChecked(dsRes <= 0);
	dsHd->SetChecked(dsRes == 1);
	dsFhd->SetChecked(dsRes == 2);
	dsQhd->SetChecked(dsRes == 3);

	showStats->SetChecked(Settings::Get("SHOW_STATS") == 1);
	hideToTray->SetChecked(Settings::Get("MINIMIZE_TASKBAR") == 1);
	savePresets->SetChecked(Settings::Get("SAVE_DEVICE_STATES") == 1);
}

void SettingsPanel::SelectTheme(Material::SettingsRow* selected)
{
	themeSystem->SetChecked(selected == themeSystem);
	themeLight->SetChecked(selected == themeLight);
	themeDark->SetChecked(selected == themeDark);
	Commit();
}

void SettingsPanel::SelectResolution(Material::SettingsRow* selected)
{
	dsSd->SetChecked(selected == dsSd);
	dsHd->SetChecked(selected == dsHd);
	dsFhd->SetChecked(selected == dsFhd);
	dsQhd->SetChecked(selected == dsQhd);
	Commit();
}

void SettingsPanel::Commit()
{
	int themeMode = 0;
	if (themeLight->GetChecked()) themeMode = 1;
	else if (themeDark->GetChecked()) themeMode = 2;
	Settings::Set("THEME_MODE", themeMode);

	int dsRes = 0;
	if (dsHd->GetChecked()) dsRes = 1;
	else if (dsFhd->GetChecked()) dsRes = 2;
	else if (dsQhd->GetChecked()) dsRes = 3;
	Settings::Set("DIRECTSHOW_RESOLUTION", dsRes);

	Settings::Set("SHOW_STATS", showStats->GetChecked() ? 1 : 0);
	Settings::Set("MINIMIZE_TASKBAR", hideToTray->GetChecked() ? 1 : 0);
	Settings::Set("SAVE_DEVICE_STATES", savePresets->GetChecked() ? 1 : 0);
	Settings::Save();

	if (onChanged)
		onChanged();
}
