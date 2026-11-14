#pragma once

#include <wx/wx.h>

class SettingsDialog : public wxDialog
{
public:
	SettingsDialog(wxWindow* parent);

	int GetThemeMode() const;
	int GetDirectShowResolution() const;
	bool GetShowStats() const;
	bool GetHideToTray() const;
	bool GetSavePresets() const;

private:
	wxRadioButton* themeSystem = nullptr;
	wxRadioButton* themeLight = nullptr;
	wxRadioButton* themeDark = nullptr;

	wxRadioButton* dsSd = nullptr;
	wxRadioButton* dsHd = nullptr;
	wxRadioButton* dsFhd = nullptr;
	wxRadioButton* dsQhd = nullptr;

	wxCheckBox* showStats = nullptr;
	wxCheckBox* hideToTray = nullptr;
	wxCheckBox* savePresets = nullptr;
};
