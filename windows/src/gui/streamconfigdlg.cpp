#include "gui/streamconfigdlg.h"
#include "gui/theme.h"

#include "rtsp/manager.h"

wxDEFINE_EVENT(EVT_STREAM_CONFIG_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(EVT_STREAM_RESOLUTION_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(EVT_STREAM_FPS_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(EVT_STREAM_BITRATE_CHANGED, wxCommandEvent);

StreamConfigDlg::StreamConfigDlg(wxWindow* parent,
    const DeviceDescriptor& device,
    bool isBackCamera,
    const Config& currentConfig)
    : wxDialog(parent, wxID_ANY, "Stream Configuration", wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    SetBackgroundColour(Material::Colors().background);

    auto mainSizer = new wxBoxSizer(wxVERTICAL);

    auto title = Material::Label(this, "Stream settings", Material::TypeRole::HeadlineSmall);
    mainSizer->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(24));

    auto subtitle = Material::Label(this, "Tune video quality and camera behaviour.",
        Material::TypeRole::BodySmall, Material::Colors().onSurfaceVariant);
    mainSizer->Add(subtitle, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(24));
    mainSizer->AddSpacer(FromDIP(16));

    // ---------------------------------------------------------
    // GROUP 1: VIDEO QUALITY
    // ---------------------------------------------------------
    auto qualityBox = new Material::Section(this, "Video quality");
    qualityBox->SetBackdrop(Material::Colors().background);

    auto qualityGrid = new wxFlexGridSizer(2, FromDIP(12), FromDIP(16));
    qualityGrid->AddGrowableCol(1, 1);

    // 1. Resolution
    qualityGrid->Add(Material::Label(qualityBox, "Resolution:", Material::TypeRole::BodyMedium),
        0, wxALIGN_CENTER_VERTICAL);
    resChoice = new wxChoice(qualityBox, wxID_ANY);
    const auto& resolutions = isBackCamera ? device.backResolutions() : device.frontResolutions();
    for (const auto& res : resolutions)
    {
        resChoice->Append(wxString::Format("%d x %d", res.first, res.second));
    }
    if (currentConfig.resIndex >= 0 && currentConfig.resIndex < resChoice->GetCount())
        resChoice->SetSelection(currentConfig.resIndex);
    Material::StyleChoice(resChoice);
    resChoice->Bind(wxEVT_CHOICE, &StreamConfigDlg::OnResolutionChanged, this);
    qualityGrid->Add(resChoice, 1, wxEXPAND);

    // 2. Framerate
    qualityGrid->Add(Material::Label(qualityBox, "Framerate:", Material::TypeRole::BodyMedium),
        0, wxALIGN_CENTER_VERTICAL);
    fpsChoice = new wxChoice(qualityBox, wxID_ANY);
    std::vector<int> fpsOptions = { 15, 24, 30, 60 };
    int fpsSel = 2;
    for (size_t i = 0; i < fpsOptions.size(); i++)
    {
        fpsChoice->Append(std::to_string(fpsOptions[i]) + " FPS");
        if (fpsOptions[i] == currentConfig.fps) fpsSel = i;
    }
    fpsChoice->SetSelection(fpsSel);
    Material::StyleChoice(fpsChoice);
    fpsChoice->Bind(wxEVT_CHOICE, &StreamConfigDlg::OnFpsChanged, this);
    qualityGrid->Add(fpsChoice, 1, wxEXPAND);

    // 3. Bitrate
    qualityGrid->Add(Material::Label(qualityBox, "Bitrate mode:", Material::TypeRole::BodyMedium),
        0, wxALIGN_TOP | wxTOP, FromDIP(5));

    auto bitrateContainer = new wxBoxSizer(wxVERTICAL);

    auto radioSizer = new wxBoxSizer(wxHORIZONTAL);
    rbStatic = new wxRadioButton(qualityBox, wxID_ANY, "Static (Constant)", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    rbAdaptive = new wxRadioButton(qualityBox, wxID_ANY, "Adaptive (Variable)");
    Material::StyleRadioButton(rbStatic);
    Material::StyleRadioButton(rbAdaptive);

    rbStatic->SetValue(!currentConfig.adaptiveBitrate);
    rbAdaptive->SetValue(currentConfig.adaptiveBitrate);

    rbStatic->Bind(wxEVT_RADIOBUTTON, &StreamConfigDlg::OnBitrateModeChanged, this);
    rbAdaptive->Bind(wxEVT_RADIOBUTTON, &StreamConfigDlg::OnBitrateModeChanged, this);

    radioSizer->Add(rbStatic, 0, wxRIGHT, FromDIP(15));
    radioSizer->Add(rbAdaptive, 0);
    bitrateContainer->Add(radioSizer, 0, wxBOTTOM, FromDIP(10));

    staticBitratePanel = new wxPanel(qualityBox, wxID_ANY);
    staticBitratePanel->SetBackgroundColour(qualityBox->GetBackgroundColour());
    auto staticSizer = new wxBoxSizer(wxHORIZONTAL);

    int defStatic = currentConfig.bitrate > 0 ? currentConfig.bitrate : RTSP::DEFAULT_STATIC_BITRATE;

    staticLabel = Material::Label(staticBitratePanel, FormatBitrate(defStatic), Material::TypeRole::LabelMedium);
    staticLabel->SetWindowStyleFlag(wxALIGN_RIGHT);
    staticSlider = new wxSlider(staticBitratePanel, wxID_ANY, defStatic, RTSP::MIN_BITRATE, RTSP::MAX_BITRATE);
    Material::StyleSlider(staticSlider);
    staticSlider->Bind(wxEVT_SLIDER, &StreamConfigDlg::OnBitrateSliderChanged, this);

    staticSizer->Add(staticSlider, 1, wxEXPAND | wxRIGHT, FromDIP(8));
    staticSizer->Add(staticLabel, 0, wxALIGN_CENTER_VERTICAL);
    staticBitratePanel->SetSizer(staticSizer);

    bitrateContainer->Add(staticBitratePanel, 1, wxEXPAND);

    adaptiveBitratePanel = new wxPanel(qualityBox, wxID_ANY);
    adaptiveBitratePanel->SetBackgroundColour(qualityBox->GetBackgroundColour());
    auto adaptiveSizer = new wxFlexGridSizer(2, FromDIP(6), FromDIP(8));
    adaptiveSizer->AddGrowableCol(1, 1);

    int defMin = currentConfig.minBitrate > 0 ? currentConfig.minBitrate : RTSP::DEFAULT_MIN_BITRATE;
    int defMax = currentConfig.maxBitrate > 0 ? currentConfig.maxBitrate : RTSP::DEFAULT_MAX_BITRATE;

    adaptiveSizer->Add(Material::Label(adaptiveBitratePanel, "Min:", Material::TypeRole::BodyMedium), 0, wxALIGN_CENTER_VERTICAL);
    minSlider = new wxSlider(adaptiveBitratePanel, wxID_ANY, defMin, RTSP::MIN_BITRATE, RTSP::MAX_BITRATE);
    Material::StyleSlider(minSlider);
    minLabel = Material::Label(adaptiveBitratePanel, FormatBitrate(defMin), Material::TypeRole::LabelMedium);
    minLabel->SetWindowStyleFlag(wxALIGN_RIGHT);
    minSlider->Bind(wxEVT_SLIDER, &StreamConfigDlg::OnBitrateSliderChanged, this);
    adaptiveSizer->Add(minSlider, 1, wxEXPAND);
    adaptiveSizer->Add(minLabel, 0, wxALIGN_CENTER_VERTICAL);

    adaptiveSizer->Add(Material::Label(adaptiveBitratePanel, "Max:", Material::TypeRole::BodyMedium), 0, wxALIGN_CENTER_VERTICAL);
    maxSlider = new wxSlider(adaptiveBitratePanel, wxID_ANY, defMax, RTSP::MIN_BITRATE, RTSP::MAX_BITRATE);
    Material::StyleSlider(maxSlider);
    maxLabel = Material::Label(adaptiveBitratePanel, FormatBitrate(defMax), Material::TypeRole::LabelMedium);
    maxLabel->SetWindowStyleFlag(wxALIGN_RIGHT);
    maxSlider->Bind(wxEVT_SLIDER, &StreamConfigDlg::OnBitrateSliderChanged, this);
    adaptiveSizer->Add(maxSlider, 1, wxEXPAND);
    adaptiveSizer->Add(maxLabel, 0, wxALIGN_CENTER_VERTICAL);

    adaptiveBitratePanel->SetSizer(adaptiveSizer);
    bitrateContainer->Add(adaptiveBitratePanel, 1, wxEXPAND);

    qualityGrid->Add(bitrateContainer, 1, wxEXPAND);

    // 4. Codec
    qualityGrid->Add(Material::Label(qualityBox, "Codec:", Material::TypeRole::BodyMedium),
        0, wxALIGN_CENTER_VERTICAL);
    codecChoice = new wxChoice(qualityBox, wxID_ANY);
    codecChoice->Append("H.264 (Standard)");
    codecChoice->Append("H.265 (HEVC)");
    codecChoice->SetSelection(currentConfig.h265Enabled ? 1 : 0);
    Material::StyleChoice(codecChoice);
    codecChoice->Bind(wxEVT_CHOICE, &StreamConfigDlg::OnSimpleSettingChanged, this);
    qualityGrid->Add(codecChoice, 1, wxEXPAND);

    qualityBox->Body()->Add(qualityGrid, 1, wxEXPAND);

    // Initial visibility update
    UpdateBitrateUI();

    mainSizer->Add(qualityBox, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
    mainSizer->AddSpacer(FromDIP(16));

    // ---------------------------------------------------------
    // GROUP 2: CAMERA
    // ---------------------------------------------------------
    auto hwBox = new Material::Section(this, "Camera settings");
    hwBox->SetBackdrop(Material::Colors().background);

    auto hwGrid = new wxFlexGridSizer(2, FromDIP(12), FromDIP(16));
    hwGrid->AddGrowableCol(1, 1);

    hwGrid->Add(Material::Label(hwBox, "Focus mode:", Material::TypeRole::BodyMedium),
        0, wxALIGN_CENTER_VERTICAL);
    focusChoice = new wxChoice(hwBox, wxID_ANY);
    focusChoice->Append("Continuous Video (Auto)");
    focusChoice->Append("Fixed / Infinity");
    focusChoice->SetSelection(currentConfig.focusMode);
    Material::StyleChoice(focusChoice);
    focusChoice->Bind(wxEVT_CHOICE, &StreamConfigDlg::OnSimpleSettingChanged, this);
    hwGrid->Add(focusChoice, 1, wxEXPAND);

    hwGrid->Add(Material::Label(hwBox, "Stabilization:", Material::TypeRole::BodyMedium),
        0, wxALIGN_CENTER_VERTICAL);
    stabilizationCheck = new wxCheckBox(hwBox, wxID_ANY, "Enable EIS");
    Material::StyleCheckBox(stabilizationCheck);
    stabilizationCheck->SetValue(currentConfig.stabilizationEnabled);
    stabilizationCheck->Bind(wxEVT_CHECKBOX, &StreamConfigDlg::OnSimpleSettingChanged, this);
    hwGrid->Add(stabilizationCheck, 0, wxALIGN_CENTER_VERTICAL);

    flashCheck = new wxCheckBox(hwBox, wxID_ANY, "Enable flashlight (torch)");
    Material::StyleCheckBox(flashCheck);
    flashCheck->SetValue(currentConfig.flashEnabled);
    flashCheck->Bind(wxEVT_CHECKBOX, &StreamConfigDlg::OnSimpleSettingChanged, this);
    hwGrid->Add(new wxPanel(hwBox), 0);
    hwGrid->Add(flashCheck, 0, wxALIGN_CENTER_VERTICAL);

    hwBox->Body()->Add(hwGrid, 1, wxEXPAND);

    mainSizer->Add(hwBox, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
    mainSizer->AddSpacer(FromDIP(20));

    // ---------------------------------------------------------
    // BUTTONS
    // ---------------------------------------------------------
    auto btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    auto closeBtn = new Material::PillButton(this, "Done", Material::PillButton::Variant::Filled);
    closeBtn->SetBackdrop(Material::Colors().background);
    closeBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CallAfter([this]() { EndModal(wxID_OK); }); });
    btnSizer->Add(closeBtn, 0);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(24));

    SetSizerAndFit(mainSizer);
    SetMinSize(FromDIP(wxSize(620, 500)));
    CenterOnParent();
}

StreamConfigDlg::~StreamConfigDlg() {}

wxString StreamConfigDlg::FormatBitrate(int kbps)
{
    if (kbps >= 1000) return wxString::Format("%.1f Mbps", kbps / 1000.0f);
    return wxString::Format("%d Kbps", kbps);
}

// --- Accessors for Parent ---
bool StreamConfigDlg::IsAdaptiveBitrate() const { return rbAdaptive->GetValue(); }
int StreamConfigDlg::GetStaticBitrate() const { return staticSlider->GetValue(); }
int StreamConfigDlg::GetMinBitrate() const { return minSlider->GetValue(); }
int StreamConfigDlg::GetMaxBitrate() const { return maxSlider->GetValue(); }

// --- Event Handlers ---

void StreamConfigDlg::UpdateBitrateUI()
{
    bool adaptive = rbAdaptive->GetValue();

    staticBitratePanel->Show(!adaptive);
    adaptiveBitratePanel->Show(adaptive);

    // We must call Layout to reflow the dialog because sizes changed
    Layout();

    // Resize dialog if content grows significantly
    Fit();
}

void StreamConfigDlg::OnBitrateModeChanged(wxCommandEvent& event)
{
    UpdateBitrateUI();

    // Notify parent that configuration structure changed
    wxCommandEvent evt(EVT_STREAM_BITRATE_CHANGED);
    evt.SetEventObject(this);
    ProcessWindowEvent(evt);
}

void StreamConfigDlg::OnBitrateSliderChanged(wxCommandEvent& event)
{
    // Update labels dynamically
    if (event.GetEventObject() == staticSlider)
    {
        staticLabel->SetLabel(FormatBitrate(staticSlider->GetValue()));
    }
    else if (event.GetEventObject() == minSlider)
    {
        // Validation: Min should not exceed Max
        if (minSlider->GetValue() > maxSlider->GetValue())
            maxSlider->SetValue(minSlider->GetValue());

        minLabel->SetLabel(FormatBitrate(minSlider->GetValue()));
        maxLabel->SetLabel(FormatBitrate(maxSlider->GetValue())); // Update Max too if forced
    }
    else if (event.GetEventObject() == maxSlider)
    {
        // Validation: Max should not be lower than Min
        if (maxSlider->GetValue() < minSlider->GetValue())
            minSlider->SetValue(maxSlider->GetValue());

        maxLabel->SetLabel(FormatBitrate(maxSlider->GetValue()));
        minLabel->SetLabel(FormatBitrate(minSlider->GetValue())); // Update Min too if forced
    }

    // Fire generic event
    wxCommandEvent evt(EVT_STREAM_BITRATE_CHANGED);
    evt.SetEventObject(this);
    ProcessWindowEvent(evt);
}

void StreamConfigDlg::OnResolutionChanged(wxCommandEvent& event)
{
    wxCommandEvent evt(EVT_STREAM_RESOLUTION_CHANGED);
    evt.SetEventObject(this);
    evt.SetString(resChoice->GetStringSelection());
    ProcessWindowEvent(evt);
}

void StreamConfigDlg::OnFpsChanged(wxCommandEvent& event)
{
    long fps = 0;
    fpsChoice->GetStringSelection().BeforeFirst(' ').ToLong(&fps);
    wxCommandEvent evt(EVT_STREAM_FPS_CHANGED);
    evt.SetEventObject(this);
    evt.SetInt((int)fps);
    ProcessWindowEvent(evt);
}

void StreamConfigDlg::OnSimpleSettingChanged(wxCommandEvent& event)
{
    wxCommandEvent evt(EVT_STREAM_CONFIG_CHANGED);
    evt.SetEventObject(this);
    ProcessWindowEvent(evt);
}
