#include "ui/setup_dialog.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <array>

namespace eexi_cii {
namespace {

constexpr const char* kDisclaimer =
    "Operational awareness only - not for official regulatory submission.";

struct ShipTypeChoice {
    const char* label;
    ShipType type;
};

struct FuelTypeChoice {
    const char* label;
    FuelType type;
};

constexpr std::array<ShipTypeChoice, 14> kShipTypes{{
    {"Bulk carrier", ShipType::BulkCarrier},
    {"Gas carrier >= 65,000 DWT", ShipType::GasCarrierGte65k},
    {"Gas carrier < 65,000 DWT", ShipType::GasCarrierLt65k},
    {"Tanker", ShipType::Tanker},
    {"Container ship", ShipType::ContainerShip},
    {"General cargo", ShipType::GeneralCargo},
    {"Refrigerated cargo", ShipType::RefrigeratedCargo},
    {"Combination carrier", ShipType::CombinationCarrier},
    {"LNG carrier >= 100,000 DWT", ShipType::LngCarrierGte100k},
    {"LNG carrier < 100,000 DWT", ShipType::LngCarrierLt100k},
    {"Ro-ro vehicle carrier", ShipType::RoroVehicleCarrier},
    {"Ro-ro cargo", ShipType::RoroCargo},
    {"Ro-ro passenger", ShipType::RoroPassenger},
    {"Cruise passenger", ShipType::CruisePassenger},
}};

constexpr std::array<FuelTypeChoice, 5> kFuelTypes{{
    {"HFO", FuelType::Hfo},
    {"LFO", FuelType::Lfo},
    {"MDO", FuelType::Mdo},
    {"LNG", FuelType::Lng},
    {"Methanol", FuelType::Methanol},
}};

wxSpinCtrlDouble* add_number(
    wxWindow* parent,
    wxFlexGridSizer* grid,
    const wxString& label,
    const double min_value,
    const double max_value,
    const double increment,
    const int digits
) {
    grid->Add(new wxStaticText(parent, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    auto* control = new wxSpinCtrlDouble(parent, wxID_ANY);
    control->SetRange(min_value, max_value);
    control->SetIncrement(increment);
    control->SetDigits(digits);
    grid->Add(control, 1, wxEXPAND | wxALL, 4);
    return control;
}

wxTextCtrl* add_text(wxWindow* parent, wxFlexGridSizer* grid, const wxString& label) {
    grid->Add(new wxStaticText(parent, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    auto* control = new wxTextCtrl(parent, wxID_ANY);
    grid->Add(control, 1, wxEXPAND | wxALL, 4);
    return control;
}

wxFlexGridSizer* two_column_grid() {
    auto* grid = new wxFlexGridSizer(2, 8, 8);
    grid->AddGrowableCol(1, 1);
    return grid;
}

int ship_type_selection(const ShipType type) {
    for (std::size_t index = 0; index < kShipTypes.size(); ++index) {
        if (kShipTypes[index].type == type) {
            return static_cast<int>(index);
        }
    }
    return 0;
}

int fuel_type_selection(const FuelType type) {
    for (std::size_t index = 0; index < kFuelTypes.size(); ++index) {
        if (kFuelTypes[index].type == type) {
            return static_cast<int>(index);
        }
    }
    return 0;
}

int target_year_selection(const int year) {
    if (year >= 2023 && year <= 2026) {
        return year - 2022;
    }
    return 0;
}

int target_year_from_selection(const int selection) {
    if (selection >= 1 && selection <= 4) {
        return 2022 + selection;
    }
    return 0;
}

} // namespace

SetupDialog::SetupDialog(wxWindow* parent, const ProfileSettings& initial_settings)
    : wxDialog(parent, wxID_ANY, "EEXI-CII Setup", wxDefaultPosition, wxSize(520, 520),
          wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    auto* root = new wxBoxSizer(wxVERTICAL);
    auto* notebook = new wxNotebook(this, wxID_ANY);

    auto* vessel_panel = new wxPanel(notebook);
    auto* vessel_sizer = new wxBoxSizer(wxVERTICAL);
    auto* vessel_grid = two_column_grid();
    m_name = add_text(vessel_panel, vessel_grid, "Ship name");
    m_imo_number = add_text(vessel_panel, vessel_grid, "IMO number");

    vessel_grid->Add(new wxStaticText(vessel_panel, wxID_ANY, "Ship type"), 0,
        wxALIGN_CENTER_VERTICAL | wxALL, 4);
    m_ship_type = new wxChoice(vessel_panel, wxID_ANY);
    for (const auto& item : kShipTypes) {
        m_ship_type->Append(item.label);
    }
    vessel_grid->Add(m_ship_type, 1, wxEXPAND | wxALL, 4);

    m_gt = add_number(vessel_panel, vessel_grid, "GT", 0.0, 1000000.0, 100.0, 1);
    m_dwt = add_number(vessel_panel, vessel_grid, "DWT", 0.0, 1000000.0, 100.0, 1);
    m_admiralty_coefficient =
        add_number(vessel_panel, vessel_grid, "Admiralty coefficient", 0.0, 10000.0, 1.0, 1);
    m_sfoc = add_number(vessel_panel, vessel_grid, "SFOC g/kWh", 0.0, 400.0, 1.0, 1);

    vessel_grid->Add(new wxStaticText(vessel_panel, wxID_ANY, "Fuel type"), 0,
        wxALIGN_CENTER_VERTICAL | wxALL, 4);
    m_fuel_type = new wxChoice(vessel_panel, wxID_ANY);
    for (const auto& item : kFuelTypes) {
        m_fuel_type->Append(item.label);
    }
    vessel_grid->Add(m_fuel_type, 1, wxEXPAND | wxALL, 4);

    m_displacement =
        add_number(vessel_panel, vessel_grid, "Displacement tonnes", 0.0, 1000000.0, 100.0, 1);
    vessel_sizer->Add(vessel_grid, 1, wxEXPAND | wxALL, 12);
    vessel_panel->SetSizer(vessel_sizer);
    notebook->AddPage(vessel_panel, "Vessel");

    auto* eexi_panel = new wxPanel(notebook);
    auto* eexi_sizer = new wxBoxSizer(wxVERTICAL);
    auto* eexi_grid = two_column_grid();
    m_installed_power_mco =
        add_number(eexi_panel, eexi_grid, "Installed power MCO kW", 0.0, 200000.0, 100.0, 1);
    m_reference_speed =
        add_number(eexi_panel, eexi_grid, "Reference speed knots", 0.0, 60.0, 0.1, 2);
    eexi_sizer->Add(eexi_grid, 0, wxEXPAND | wxALL, 12);
    eexi_sizer->Add(new wxStaticText(eexi_panel, wxID_ANY, kDisclaimer), 0,
        wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    eexi_panel->SetSizer(eexi_sizer);
    notebook->AddPage(eexi_panel, "EEXI");

    auto* advanced_panel = new wxPanel(notebook);
    auto* advanced_sizer = new wxBoxSizer(wxVERTICAL);
    auto* advanced_grid = two_column_grid();
    m_sog_threshold =
        add_number(advanced_panel, advanced_grid, "SOG threshold knots", 0.0, 20.0, 0.1, 2);
    advanced_grid->Add(new wxStaticText(advanced_panel, wxID_ANY, "Target year"), 0,
        wxALIGN_CENTER_VERTICAL | wxALL, 4);
    m_target_year = new wxChoice(advanced_panel, wxID_ANY);
    m_target_year->Append("Auto");
    m_target_year->Append("2023");
    m_target_year->Append("2024");
    m_target_year->Append("2025");
    m_target_year->Append("2026");
    advanced_grid->Add(m_target_year, 1, wxEXPAND | wxALL, 4);
    m_ytd_seed_co2 =
        add_number(advanced_panel, advanced_grid, "YTD seed CO2 tonnes", 0.0, 10000000.0, 1.0, 2);
    m_ytd_seed_distance =
        add_number(advanced_panel, advanced_grid, "YTD seed distance nm", 0.0, 10000000.0, 1.0, 2);
    advanced_sizer->Add(advanced_grid, 0, wxEXPAND | wxALL, 12);
    advanced_panel->SetSizer(advanced_sizer);
    notebook->AddPage(advanced_panel, "Advanced");

    root->Add(notebook, 1, wxEXPAND | wxALL, 8);
    auto* buttons = CreateSeparatedButtonSizer(wxOK | wxCANCEL);
    root->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    SetSizerAndFit(root);

    Bind(wxEVT_BUTTON, &SetupDialog::on_ok, this, wxID_OK);
    populate_from_settings(initial_settings);
}

ProfileSettings SetupDialog::settings() const {
    ProfileSettings value;
    value.has_profile = true;
    value.profile.name = m_name->GetValue().ToStdString();
    value.profile.imo_number = m_imo_number->GetValue().ToStdString();
    value.profile.ship_type = kShipTypes[static_cast<std::size_t>(m_ship_type->GetSelection())].type;
    value.profile.gt = m_gt->GetValue();
    value.profile.dwt = m_dwt->GetValue();
    value.profile.admiralty_coefficient = m_admiralty_coefficient->GetValue();
    value.profile.sfoc = m_sfoc->GetValue();
    value.profile.fuel_type = kFuelTypes[static_cast<std::size_t>(m_fuel_type->GetSelection())].type;
    value.profile.displacement = m_displacement->GetValue();
    value.profile.installed_power_mco = m_installed_power_mco->GetValue();
    value.profile.reference_speed = m_reference_speed->GetValue();
    value.sog_threshold = m_sog_threshold->GetValue();
    value.target_year_override = target_year_from_selection(m_target_year->GetSelection());
    value.ytd_seed_co2_tonnes = m_ytd_seed_co2->GetValue();
    value.ytd_seed_distance_nm = m_ytd_seed_distance->GetValue();
    return value;
}

void SetupDialog::populate_from_settings(const ProfileSettings& settings) {
    m_name->SetValue(wxString::FromUTF8(settings.profile.name.c_str()));
    m_imo_number->SetValue(wxString::FromUTF8(settings.profile.imo_number.c_str()));
    m_ship_type->SetSelection(ship_type_selection(settings.profile.ship_type));
    m_gt->SetValue(settings.profile.gt);
    m_dwt->SetValue(settings.profile.dwt);
    m_admiralty_coefficient->SetValue(settings.profile.admiralty_coefficient);
    m_sfoc->SetValue(settings.profile.sfoc);
    m_fuel_type->SetSelection(fuel_type_selection(settings.profile.fuel_type));
    m_displacement->SetValue(settings.profile.displacement);
    m_installed_power_mco->SetValue(settings.profile.installed_power_mco);
    m_reference_speed->SetValue(settings.profile.reference_speed);
    m_sog_threshold->SetValue(settings.sog_threshold);
    m_target_year->SetSelection(target_year_selection(settings.target_year_override));
    m_ytd_seed_co2->SetValue(settings.ytd_seed_co2_tonnes);
    m_ytd_seed_distance->SetValue(settings.ytd_seed_distance_nm);
}

void SetupDialog::on_ok(wxCommandEvent&) {
    const ProfileSettings value = settings();
    if (value.setup_required()) {
        wxMessageBox(
            "Complete the required vessel profile fields before saving.",
            "EEXI-CII Setup",
            wxOK | wxICON_WARNING,
            this
        );
        return;
    }

    EndModal(wxID_OK);
}

} // namespace eexi_cii
