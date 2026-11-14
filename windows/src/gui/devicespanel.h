#pragma once

#include <wx/wx.h>
#include <vector>
#include "net/devicedescriptor.h"

class DevicesPanel : public wxPanel
{
public:
	DevicesPanel(wxWindow* parent);

	void SetDevices(const std::vector<DeviceDescriptor>& devices);

private:
	wxPanel* container = nullptr;
	wxBoxSizer* listSizer = nullptr;
};
