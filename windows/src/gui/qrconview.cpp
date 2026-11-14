#include "gui/qrconview.h"
#include "gui/theme.h"

#include <cstring>

QrconView::QrconView(std::string name, std::string address, std::string port, wxSize displaySize)
    : wxDialog(nullptr, wxID_ANY, "Connect with QR code", wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE)
{
    SetBackgroundColour(Material::Colors().background);

    auto mainSizer = new wxBoxSizer(wxVERTICAL);

    auto title = Material::Label(this, "Connect your phone", Material::TypeRole::HeadlineSmall);
    mainSizer->Add(title, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, FromDIP(28));

    auto subtitle = Material::Label(this, "Scan this code with the VCamdroid Android app",
        Material::TypeRole::BodySmall, Material::Colors().onSurfaceVariant);
    mainSizer->Add(subtitle, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, FromDIP(6));
    mainSizer->AddSpacer(FromDIP(20));

    auto qrcode = GenerateQRCode(address + ":" + port);
    auto image = GenerateImageFromQR(qrcode);

    auto qrCard = new Material::Card(this, 20, Material::Shape::Large);
    qrCard->SetBackdrop(Material::Colors().background);
    qrCard->SetFill(*wxWHITE);

    // Nearest-neighbour keeps the QR modules crisp when scaling up.
    auto canvas = new wxStaticBitmap(qrCard, wxID_ANY,
        wxBitmap(image.Scale(displaySize, wxIMAGE_QUALITY_NEAREST)));
    canvas->SetBackgroundColour(*wxWHITE);
    qrCard->Content()->Add(canvas, 0, wxALIGN_CENTER);
    mainSizer->Add(qrCard, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, FromDIP(40));

    mainSizer->AddSpacer(FromDIP(20));

    auto details = new Material::Card(this, 16, Material::Shape::Medium);
    details->SetBackdrop(Material::Colors().background);
    details->SetFill(Material::Colors().surfaceContainerHigh);

    details->Content()->Add(Material::Label(details, name, Material::TypeRole::TitleSmall),
        0, wxBOTTOM, FromDIP(6));
    details->Content()->Add(Material::Label(details, wxString::Format("Address: %s", address),
        Material::TypeRole::BodySmall, Material::Colors().onSurfaceVariant), 0);
    details->Content()->Add(Material::Label(details, wxString::Format("Port: %s", port),
        Material::TypeRole::BodySmall, Material::Colors().onSurfaceVariant), 0, wxTOP, FromDIP(2));

    mainSizer->Add(details, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(40));
    mainSizer->AddSpacer(FromDIP(20));

    auto buttonRow = new wxBoxSizer(wxHORIZONTAL);
    buttonRow->AddStretchSpacer(1);
    auto closeButton = new Material::PillButton(this, "Done", Material::PillButton::Variant::Filled);
    closeButton->SetBackdrop(Material::Colors().background);
    closeButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CallAfter([this]() { EndModal(wxID_OK); }); });
    buttonRow->Add(closeButton, 0);
    mainSizer->Add(buttonRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(24));

    SetSizerAndFit(mainSizer);
    CenterOnParent();
}

qrcodegen::QrCode QrconView::GenerateQRCode(std::string data)
{
    return qrcodegen::QrCode::encodeText(data.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
}

wxImage QrconView::GenerateImageFromQR(const qrcodegen::QrCode& qrcode)
{
    const int size = qrcode.getSize();

    // The QR specification requires a quiet zone of at least 4 modules.
    // Without it ML Kit (and many other scanners) fail silently. Use a
    // generous margin so the built-in dialog is as easy to scan as the
    // fullscreen fallback that was needed before.
    const int border = 8;
    const int total = size + border * 2;

    // Let wxImage own the pixel buffer. (The old code allocated with new[]
    // and handed it to wxImage, which frees it with free().)
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

    return image;
}
