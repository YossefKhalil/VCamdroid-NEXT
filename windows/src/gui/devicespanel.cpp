#include "gui/devicespanel.h"
#include "gui/theme.h"

#include <algorithm>

DevicesPanel::DevicesPanel(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
{
	SetBackgroundColour(Material::Colors().background);

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->AddSpacer(FromDIP(20));

	container = new wxPanel(this, wxID_ANY);
	container->SetBackgroundColour(Material::Colors().background);
	listSizer = new wxBoxSizer(wxVERTICAL);
	container->SetSizer(listSizer);

	mainSizer->Add(container, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(24));
	SetSizer(mainSizer);
}

void DevicesPanel::SetDevices(const std::vector<DeviceDescriptor>& devices)
{
	container->DestroyChildren();
	listSizer->Clear(false);

	if (devices.empty())
	{
		auto empty = Material::Label(container, "No devices connected. Scan the QR code to pair a phone.",
			Material::TypeRole::BodyMedium, Material::Colors().onSurfaceVariant);
		listSizer->Add(empty, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, FromDIP(40));
	}
	else
	{
		auto getMaxResString = [](const std::vector<DeviceDescriptor::Resolution>& resList) -> wxString {
			if (resList.empty()) return "-";
			auto maxRes = *std::max_element(resList.begin(), resList.end(),
				[](const DeviceDescriptor::Resolution& a, const DeviceDescriptor::Resolution& b) {
					return (a.first * a.second) < (b.first * b.second);
				});
			return wxString::Format("%dx%d", maxRes.first, maxRes.second);
		};

		for (const auto& dev : devices)
		{
			auto card = new Material::DeviceCard(container, dev.name(), dev.url(),
				getMaxResString(dev.backResolutions()), getMaxResString(dev.frontResolutions()));
			listSizer->Add(card, 0, wxEXPAND | wxBOTTOM, FromDIP(8));
		}
	}

	container->Layout();
	Layout();
}
