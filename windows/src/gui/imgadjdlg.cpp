#include "gui/imgadjdlg.h"
#include "gui/theme.h"

wxDEFINE_EVENT(EVT_FILTER_PARAM_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(EVT_FILTER_SWITCH_CHANGED, wxCommandEvent);

ImgAdjDlg::ImgAdjDlg(wxWindow* parent,
    const DeviceDescriptor& deviceDesc,
    const std::map<std::string, int>& initialSliderValues,
    std::string initialActiveFilter)
    : wxDialog(parent, wxID_ANY, "Image Adjustments", wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    SetBackgroundColour(Material::Colors().background);

    auto mainSizer = new wxBoxSizer(wxVERTICAL);

    auto title = Material::Label(this, "Image adjustments", Material::TypeRole::HeadlineSmall);
    mainSizer->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(24));

    auto subtitle = Material::Label(this, "50% is neutral. Lower values reduce the effect.",
        Material::TypeRole::BodySmall, Material::Colors().onSurfaceVariant);
    mainSizer->Add(subtitle, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(24));
    mainSizer->AddSpacer(FromDIP(16));

    // Containers
    auto corrections = new Material::Section(this, "Color corrections");
    corrections->SetBackdrop(Material::Colors().background);
    auto adjustmentsGrid = new wxFlexGridSizer(3, FromDIP(10), FromDIP(12));
    adjustmentsGrid->AddGrowableCol(1, 1);

    auto filters = new Material::Section(this, "Active filters");
    filters->SetBackdrop(Material::Colors().background);
    auto filtersGrid = new wxFlexGridSizer(2, FromDIP(10), FromDIP(12));
    filtersGrid->AddGrowableCol(1, 1);

    bool hasSliders = false;
    bool hasDropdowns = false;

    // Dynamic generation loop: iterate through the filters provided by the device.
    for (const auto& [category, filterList] : deviceDesc.filters())
    {
        // Skip the "None" category
        if (category == Video::Filter::Category::NONE)
            continue;

        // CASE A: Color category -> create sliders
        if (category == Video::Filter::Category::CORRECTION)
        {
            for (const auto& filterName : filterList)
            {
                int startValue = 50; // Default
                if (initialSliderValues.count(filterName))
                {
                    startValue = initialSliderValues.at(filterName);
                }

                AddSlider(corrections, adjustmentsGrid, filterName, startValue);
                hasSliders = true;
            }
        }
        // CASE B: All other categories -> create dropdowns
        else
        {
            std::string catName;
            switch (category)
            {
                case Video::Filter::Category::DISTORTION: catName = "Distortion"; break;
                case Video::Filter::Category::ARTISTIC: catName = "Artistic"; break;
                case Video::Filter::Category::EFFECT: catName = "Effects"; break;
                default: catName = "Other"; break;
            }

            auto label = Material::Label(filters, catName + ":", Material::TypeRole::BodyMedium);

            wxArrayString choices;
            choices.Add("None");

            int selectionIndex = 0;

            for (size_t i = 0; i < filterList.size(); i++)
            {
                std::string fName = filterList[i];
                choices.Add(fName);

                if (fName == initialActiveFilter)
                {
                    selectionIndex = static_cast<int>(i) + 1;
                }
            }

            auto choice = new wxChoice(filters, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices);
            Material::StyleChoice(choice);
            choice->SetSelection(selectionIndex);

            choice->Bind(wxEVT_CHOICE, [this, category, choice](wxCommandEvent&)
            {
                wxCommandEvent evt(EVT_FILTER_SWITCH_CHANGED);
                evt.SetEventObject(this);
                evt.SetInt(static_cast<int>(category));
                evt.SetString(choice->GetStringSelection());
                ProcessWindowEvent(evt);
            });

            categoryChoices[static_cast<int>(category)] = choice;

            filtersGrid->Add(label, 0, wxALIGN_CENTER_VERTICAL);
            filtersGrid->Add(choice, 1, wxEXPAND);
            hasDropdowns = true;
        }
    }

    if (hasSliders)
    {
        corrections->Body()->Add(adjustmentsGrid, 1, wxEXPAND);
        mainSizer->Add(corrections, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
        mainSizer->AddSpacer(FromDIP(16));
    }
    else
    {
        delete corrections;
    }

    if (hasDropdowns)
    {
        filters->Body()->Add(filtersGrid, 1, wxEXPAND);
        mainSizer->Add(filters, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
        mainSizer->AddSpacer(FromDIP(16));
    }
    else
    {
        delete filters;
    }

    // Buttons
    auto btnSizer = new wxBoxSizer(wxHORIZONTAL);
    auto resetBtn = new Material::PillButton(this, "Reset all", Material::PillButton::Variant::Outlined);
    resetBtn->SetBackdrop(Material::Colors().background);
    resetBtn->Bind(wxEVT_BUTTON, &ImgAdjDlg::OnReset, this);

    auto okBtn = new Material::PillButton(this, "Done", Material::PillButton::Variant::Filled);
    okBtn->SetBackdrop(Material::Colors().background);
    okBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CallAfter([this]() { EndModal(wxID_OK); }); });

    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(resetBtn, 0, wxRIGHT, FromDIP(10));
    btnSizer->Add(okBtn, 0);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(24));

    SetSizerAndFit(mainSizer);
    SetMinSize(FromDIP(wxSize(560, 420)));
    CenterOnParent();
}

ImgAdjDlg::~ImgAdjDlg()
{
}

void ImgAdjDlg::AddSlider(wxWindow* parent, wxSizer* sizer, const std::string& label, int initialValue)
{
    auto text = Material::Label(parent, label, Material::TypeRole::BodyMedium);

    auto slider = new wxSlider(parent, wxID_ANY, initialValue, 0, 100, wxDefaultPosition, wxSize(FromDIP(170), -1));
    Material::StyleSlider(slider);

    auto valLabel = Material::Label(parent, wxString::Format("%d%%", initialValue), Material::TypeRole::LabelMedium);

    sliders[label] = slider;
    sliderLabels[label] = valLabel;

    slider->Bind(wxEVT_SLIDER, [this, label, slider, valLabel](wxCommandEvent&)
    {
        valLabel->SetLabel(wxString::Format("%d%%", slider->GetValue()));

        wxCommandEvent evt(EVT_FILTER_PARAM_CHANGED);
        evt.SetEventObject(this);
        evt.SetString(label);
        evt.SetInt(slider->GetValue());
        ProcessWindowEvent(evt);
    });

    sizer->Add(text, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(slider, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(6));
    sizer->Add(valLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
}

void ImgAdjDlg::OnReset(wxCommandEvent& event)
{
    // 1. Reset sliders to 50%
    for (auto& [name, slider] : sliders)
    {
        slider->SetValue(50);
        sliderLabels[name]->SetLabel("50%");

        wxCommandEvent evt(EVT_FILTER_PARAM_CHANGED);
        evt.SetString(name);
        evt.SetInt(50);
        ProcessWindowEvent(evt);
    }

    // 2. Reset dropdowns to "None"
    for (auto& [cat, choice] : categoryChoices)
    {
        if (choice->GetCount() > 0)
        {
            choice->SetSelection(0);

            wxCommandEvent evt(EVT_FILTER_SWITCH_CHANGED);
            evt.SetInt(cat);
            evt.SetString("None");
            ProcessWindowEvent(evt);
        }
    }
}
