#include "gui/devicesview.h"
#include "gui/theme.h"

#include <wx/listctrl.h>
#include <algorithm> // For std::max_element

DevicesView::DevicesView(wxWindow* parent, const std::vector<DeviceDescriptor>& devices)
    : wxDialog(parent, wxID_ANY, "Connected Devices", wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    SetBackgroundColour(Material::Colors().background);

    auto mainSizer = new wxBoxSizer(wxVERTICAL);

    auto title = Material::Label(this, "Connected devices", Material::TypeRole::HeadlineSmall);
    mainSizer->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(24));

    auto subtitle = Material::Label(this, "Phones currently paired with this PC",
        Material::TypeRole::BodySmall, Material::Colors().onSurfaceVariant);
    mainSizer->Add(subtitle, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(24));
    mainSizer->AddSpacer(FromDIP(16));

    auto card = new Material::Card(this, 16, Material::Shape::Medium);
    card->SetBackdrop(Material::Colors().background);

    if (devices.empty())
    {
        auto empty = Material::Label(card, "No devices connected yet. Scan the QR code from the Android app.",
            Material::TypeRole::BodyMedium, Material::Colors().onSurfaceVariant);
        card->Content()->Add(empty, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, FromDIP(12));
    }
    else
    {
        auto list = new wxListView(card, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(660), FromDIP(260)),
            wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);

        Material::StyleListView(list);

        list->AppendColumn("#", wxLIST_FORMAT_LEFT, FromDIP(40));
        list->AppendColumn("Model name", wxLIST_FORMAT_LEFT, FromDIP(190));
        list->AppendColumn("RTSP URL", wxLIST_FORMAT_LEFT, FromDIP(260));
        list->AppendColumn("Max back", wxLIST_FORMAT_LEFT, FromDIP(100));
        list->AppendColumn("Max front", wxLIST_FORMAT_LEFT, FromDIP(100));

        auto getMaxResString = [](const std::vector<DeviceDescriptor::Resolution>& resList) -> std::string {
            if (resList.empty()) return "-";

            auto maxRes = *std::max_element(resList.begin(), resList.end(),
                [](const DeviceDescriptor::Resolution& a, const DeviceDescriptor::Resolution& b) {
                    return (a.first * a.second) < (b.first * b.second);
                });

            return std::to_string(maxRes.first) + "x" + std::to_string(maxRes.second);
        };

        for (size_t i = 0; i < devices.size(); i++)
        {
            const auto& dev = devices[i];
            long index = list->InsertItem(i, std::to_string(i + 1));

            list->SetItem(index, 1, dev.name());
            list->SetItem(index, 2, dev.url());
            list->SetItem(index, 3, getMaxResString(dev.backResolutions()));
            list->SetItem(index, 4, getMaxResString(dev.frontResolutions()));
        }

        card->Content()->Add(list, 1, wxEXPAND);
    }

    mainSizer->Add(card, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
    mainSizer->AddSpacer(FromDIP(16));

    auto buttonRow = new wxBoxSizer(wxHORIZONTAL);
    buttonRow->AddStretchSpacer(1);
    auto closeButton = new Material::PillButton(this, "Close", Material::PillButton::Variant::Filled);
    closeButton->SetBackdrop(Material::Colors().background);
    closeButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CallAfter([this]() { EndModal(wxID_OK); }); });
    buttonRow->Add(closeButton, 0);
    mainSizer->Add(buttonRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(24));

    SetSizerAndFit(mainSizer);
    SetSize(FromDIP(wxSize(820, 500)));
    SetMinSize(FromDIP(wxSize(620, 400)));
    CenterOnParent();
}
