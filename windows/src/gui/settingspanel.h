#pragma once

#include <wx/wx.h>
#include <functional>
#include "gui/theme.h"

class SettingsPanel : public wxPanel
{
public:
	SettingsPanel(wxWindow* parent, std::function<void()> onChanged);

	void Reload();

private:
	std::function<void()> onChanged;

	Material::SettingsRow* themeSystem = nullptr;
	Material::SettingsRow* themeLight = nullptr;
	Material::SettingsRow* themeDark = nullptr;

	Material::SettingsRow* dsSd = nullptr;
	Material::SettingsRow* dsHd = nullptr;
	Material::SettingsRow* dsFhd = nullptr;
	Material::SettingsRow* dsQhd = nullptr;

	Material::SettingsRow* showStats = nullptr;
	Material::SettingsRow* hideToTray = nullptr;
	Material::SettingsRow* savePresets = nullptr;

	void Commit();
	void SelectTheme(Material::SettingsRow* selected);
	void SelectResolution(Material::SettingsRow* selected);
};
