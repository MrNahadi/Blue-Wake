#include "ui/dashboard_panel.h"

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <iomanip>
#include <sstream>
#include <utility>

namespace eexi_cii {
namespace {

constexpr const char* kDisclaimer =
    "Operational awareness only - not for official regulatory submission.";

wxString number(const double value, const int precision) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return wxString::FromUTF8(out.str().c_str());
}

wxString status_text(const ProcessStatus status, const wxString& message) {
    if (!message.empty()) {
        return message;
    }

    switch (status) {
    case ProcessStatus::InvalidSentence:
        return "Waiting for valid RMC data";
    case ProcessStatus::BaselineOnly:
        return "GPS baseline acquired";
    case ProcessStatus::Excluded:
        return "Port/anchor time excluded";
    case ProcessStatus::RejectedOutlier:
        return "SOG outlier rejected";
    case ProcessStatus::Accumulated:
        return "Accumulating";
    case ProcessStatus::Error:
        return "Error";
    }

    return "Unknown";
}

wxString gps_freshness_text(const PluginSnapshot& snapshot) {
    switch (snapshot.gps_freshness) {
    case GpsFreshness::NoFix:
        return "No GPS fix";
    case GpsFreshness::Fresh:
        return "GPS live";
    case GpsFreshness::Stale:
        return "No GPS signal > 60s";
    }

    return "GPS unknown";
}

wxColour status_colour(const PluginSnapshot& snapshot, const ProcessStatus status) {
    if (status == ProcessStatus::Error || snapshot.gps_freshness == GpsFreshness::Stale) {
        return wxColour(190, 48, 42);
    }
    if (status == ProcessStatus::RejectedOutlier || status == ProcessStatus::Excluded) {
        return wxColour(188, 122, 30);
    }
    if (snapshot.gps_freshness == GpsFreshness::Fresh) {
        return wxColour(28, 126, 78);
    }
    return wxColour(92, 100, 112);
}

wxColour rating_colour(const char rating) {
    switch (rating) {
    case 'A':
        return wxColour(0, 128, 0);
    case 'B':
        return wxColour(82, 150, 0);
    case 'C':
        return wxColour(168, 130, 0);
    case 'D':
        return wxColour(198, 92, 0);
    case 'E':
        return wxColour(190, 32, 32);
    default:
        return wxColour(70, 70, 70);
    }
}

wxString ship_type_label(const ShipType type) {
    switch (type) {
    case ShipType::BulkCarrier:
        return "Bulk carrier";
    case ShipType::GasCarrierGte65k:
        return "Gas carrier >= 65,000 DWT";
    case ShipType::GasCarrierLt65k:
        return "Gas carrier < 65,000 DWT";
    case ShipType::Tanker:
        return "Tanker";
    case ShipType::ContainerShip:
        return "Container ship";
    case ShipType::GeneralCargo:
        return "General cargo";
    case ShipType::RefrigeratedCargo:
        return "Refrigerated cargo";
    case ShipType::CombinationCarrier:
        return "Combination carrier";
    case ShipType::LngCarrierGte100k:
        return "LNG carrier >= 100,000 DWT";
    case ShipType::LngCarrierLt100k:
        return "LNG carrier < 100,000 DWT";
    case ShipType::RoroVehicleCarrier:
        return "Ro-ro vehicle carrier";
    case ShipType::RoroCargo:
        return "Ro-ro cargo";
    case ShipType::RoroPassenger:
        return "Ro-ro passenger";
    case ShipType::CruisePassenger:
        return "Cruise passenger";
    case ShipType::Count:
        break;
    }
    return "Unknown ship type";
}

wxString metric_label(const VesselProfile& profile) {
    return uses_cgdist(profile.ship_type) ? "Running cgDIST" : "Running AER";
}

wxString latest_diagnostic_text(const std::vector<DiagnosticEvent>& diagnostics) {
    if (diagnostics.empty()) {
        return "No diagnostics recorded";
    }

    const DiagnosticEvent& event = diagnostics.back();
    return wxString::FromUTF8(event.code.c_str()) + ": " +
           wxString::FromUTF8(event.message.c_str());
}

wxString severity_text(const DiagnosticSeverity severity) {
    switch (severity) {
    case DiagnosticSeverity::Info:
        return "INFO";
    case DiagnosticSeverity::Warning:
        return "WARN";
    case DiagnosticSeverity::Error:
        return "ERROR";
    }
    return "UNKNOWN";
}

wxStaticText* add_metric(
    wxWindow* parent,
    wxFlexGridSizer* grid,
    const wxString& label,
    wxStaticText** label_out = nullptr
) {
    auto* label_text = new wxStaticText(parent, wxID_ANY, label);
    if (label_out != nullptr) {
        *label_out = label_text;
    }
    grid->Add(label_text, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);

    auto* value = new wxStaticText(parent, wxID_ANY, "--");
    wxFont value_font = value->GetFont();
    value_font.SetWeight(wxFONTWEIGHT_BOLD);
    value->SetFont(value_font);
    grid->Add(value, 1, wxEXPAND | wxALL, 4);
    return value;
}

wxFlexGridSizer* metric_grid() {
    auto* grid = new wxFlexGridSizer(2, 8, 12);
    grid->AddGrowableCol(1, 1);
    return grid;
}

wxButton* add_action_button(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& label,
    const std::function<void()>& action
) {
    auto* button = new wxButton(parent, wxID_ANY, label);
    button->Bind(wxEVT_BUTTON, [action](wxCommandEvent&) {
        if (action) {
            action();
        }
    });
    sizer->Add(button, 0, wxRIGHT, 6);
    return button;
}

} // namespace

DashboardPanel::DashboardPanel(wxWindow* parent, DashboardActions actions)
    : wxPanel(parent, wxID_ANY),
      m_actions(std::move(actions)),
      m_refresh_timer(this) {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* header = new wxBoxSizer(wxVERTICAL);
    m_vessel = new wxStaticText(this, wxID_ANY, "EEXI-CII Monitor");
    wxFont title_font = m_vessel->GetFont();
    title_font.SetPointSize(title_font.GetPointSize() + 3);
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_vessel->SetFont(title_font);
    header->Add(m_vessel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    m_vessel_context = new wxStaticText(this, wxID_ANY, "Configure vessel profile");
    header->Add(m_vessel_context, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);
    root->Add(header, 0, wxEXPAND);

    m_status_strip = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
    auto* strip_sizer = new wxBoxSizer(wxVERTICAL);
    m_status = new wxStaticText(m_status_strip, wxID_ANY, "Waiting for valid RMC data");
    wxFont status_font = m_status->GetFont();
    status_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_status->SetFont(status_font);
    m_gps_freshness = new wxStaticText(m_status_strip, wxID_ANY, "No GPS fix");
    strip_sizer->Add(m_status, 0, wxEXPAND | wxALL, 6);
    strip_sizer->Add(m_gps_freshness, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    m_status_strip->SetSizer(strip_sizer);
    root->Add(m_status_strip, 0, wxEXPAND | wxALL, 12);

    auto* live_box = new wxStaticBoxSizer(wxVERTICAL, this, "Live");
    auto* live_grid = metric_grid();
    m_sog = add_metric(this, live_grid, "SOG");
    m_co2_rate = add_metric(this, live_grid, "CO2 rate");
    live_box->Add(live_grid, 1, wxEXPAND | wxALL, 8);
    root->Add(live_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* cii_box = new wxStaticBoxSizer(wxVERTICAL, this, "Year to date CII");
    auto* cii_grid = metric_grid();
    m_ytd_distance = add_metric(this, cii_grid, "Distance");
    m_ytd_co2 = add_metric(this, cii_grid, "CO2");
    m_running_aer = add_metric(this, cii_grid, "Running AER", &m_running_metric_label);
    m_required_cii = add_metric(this, cii_grid, "Required CII");
    m_rating_margin = add_metric(this, cii_grid, "Margin to C/D");
    cii_box->Add(cii_grid, 1, wxEXPAND | wxALL, 8);

    m_rating_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
    auto* rating_sizer = new wxBoxSizer(wxHORIZONTAL);
    rating_sizer->Add(new wxStaticText(m_rating_panel, wxID_ANY, "CII rating"), 0,
        wxALIGN_CENTER_VERTICAL | wxALL, 8);
    m_cii_rating = new wxStaticText(m_rating_panel, wxID_ANY, "--");
    wxFont rating_font = m_cii_rating->GetFont();
    rating_font.SetPointSize(rating_font.GetPointSize() + 8);
    rating_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_cii_rating->SetFont(rating_font);
    rating_sizer->AddStretchSpacer(1);
    rating_sizer->Add(m_cii_rating, 0, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    m_rating_panel->SetSizer(rating_sizer);
    cii_box->Add(m_rating_panel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    root->Add(cii_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* voyage_box = new wxStaticBoxSizer(wxVERTICAL, this, "Current voyage");
    auto* voyage_grid = metric_grid();
    m_voyage_distance = add_metric(this, voyage_grid, "Distance");
    m_voyage_co2 = add_metric(this, voyage_grid, "CO2");
    voyage_box->Add(voyage_grid, 1, wxEXPAND | wxALL, 8);
    root->Add(voyage_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* diagnostic_box = new wxStaticBoxSizer(wxVERTICAL, this, "Diagnostics");
    m_latest_diagnostic = new wxStaticText(this, wxID_ANY, "No diagnostics recorded");
    diagnostic_box->Add(m_latest_diagnostic, 0, wxEXPAND | wxALL, 8);
    root->Add(diagnostic_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* action_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_action_button(this, action_sizer, "Settings", [this]() {
        if (m_actions.open_settings) {
            m_actions.open_settings(this);
        }
    });
    add_action_button(this, action_sizer, "Annual CSV", [this]() {
        if (m_actions.export_annual_csv) {
            m_actions.export_annual_csv(this);
        }
    });
    add_action_button(this, action_sizer, "Voyages CSV", [this]() {
        if (m_actions.export_voyage_csv) {
            m_actions.export_voyage_csv(this);
        }
    });
    add_action_button(this, action_sizer, "Diagnostics", [this]() { show_diagnostics(); });
    action_sizer->AddStretchSpacer(1);
    root->Add(action_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    root->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    auto* disclaimer = new wxStaticText(this, wxID_ANY, kDisclaimer);
    root->Add(disclaimer, 0, wxEXPAND | wxALL, 12);

    SetSizer(root);
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
        if (m_actions.refresh) {
            m_actions.refresh();
        }
    });
    m_refresh_timer.Start(10000);
}

void DashboardPanel::update(
    const PluginSnapshot& snapshot,
    const ProcessStatus status,
    const wxString& message
) {
    m_snapshot = snapshot;
    m_has_snapshot = true;

    const wxString vessel_name =
        snapshot.profile.name.empty() ? "Unnamed vessel" : wxString::FromUTF8(snapshot.profile.name.c_str());
    const wxString imo = snapshot.profile.imo_number.empty()
        ? "IMO not set"
        : "IMO " + wxString::FromUTF8(snapshot.profile.imo_number.c_str());
    m_vessel->SetLabel(vessel_name);
    m_vessel_context->SetLabel(
        imo + " | " + ship_type_label(snapshot.profile.ship_type) + " | " +
        metric_label(snapshot.profile)
    );

    m_status->SetLabel(status_text(status, message));
    m_gps_freshness->SetLabel(gps_freshness_text(snapshot));
    m_status_strip->SetBackgroundColour(status_colour(snapshot, status));

    if (snapshot.has_rmc) {
        m_sog->SetLabel(number(snapshot.last_rmc.sog_knots, 2) + " kn");
        m_co2_rate->SetLabel(number(snapshot.last_fuel_estimate.co2_rate_tonnes_hr, 3) + " t/hr");
    } else {
        m_sog->SetLabel("--");
        m_co2_rate->SetLabel("--");
    }

    m_ytd_distance->SetLabel(number(snapshot.ytd.distance_nm, 2) + " nm");
    m_ytd_co2->SetLabel(number(snapshot.ytd.co2_tonnes, 3) + " t");
    m_running_metric_label->SetLabel(metric_label(snapshot.profile));

    if (snapshot.has_aer) {
        m_running_aer->SetLabel(number(snapshot.aer_result.aer, 4));
        m_required_cii->SetLabel(number(snapshot.aer_result.required_cii, 4));
        m_cii_rating->SetLabel(wxString::Format("%c", snapshot.aer_result.rating));

        const double margin = snapshot.aer_result.boundaries.cd - snapshot.aer_result.aer;
        m_rating_margin->SetLabel(number(margin, 4));

        const wxColour colour = rating_colour(snapshot.aer_result.rating);
        m_cii_rating->SetForegroundColour(colour);
        m_rating_panel->SetBackgroundColour(colour.ChangeLightness(190));
    } else {
        m_running_aer->SetLabel("--");
        m_required_cii->SetLabel("--");
        m_rating_margin->SetLabel("--");
        m_cii_rating->SetLabel("--");
        m_cii_rating->SetForegroundColour(rating_colour('\0'));
        m_rating_panel->SetBackgroundColour(wxNullColour);
    }

    if (snapshot.current_voyage.active) {
        m_voyage_distance->SetLabel(number(snapshot.current_voyage.distance_nm, 2) + " nm");
        m_voyage_co2->SetLabel(number(snapshot.current_voyage.co2_tonnes, 3) + " t");
    } else {
        m_voyage_distance->SetLabel("No active voyage");
        m_voyage_co2->SetLabel("--");
    }

    m_latest_diagnostic->SetLabel(latest_diagnostic_text(snapshot.diagnostics));

    Layout();
    Refresh();
}

void DashboardPanel::show_diagnostics() {
    wxDialog dialog(this, wxID_ANY, "EEXI-CII Diagnostics", wxDefaultPosition, wxSize(620, 420),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    auto* root = new wxBoxSizer(wxVERTICAL);
    auto* body = new wxTextCtrl(&dialog, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);

    if (!m_has_snapshot || m_snapshot.diagnostics.empty()) {
        body->SetValue("No diagnostics recorded.");
    } else {
        wxString text;
        const auto& diagnostics = m_snapshot.diagnostics;
        for (auto it = diagnostics.rbegin(); it != diagnostics.rend(); ++it) {
            text += wxString::Format(
                "[%s] %s %s\n",
                severity_text(it->severity),
                wxString::FromUTF8(it->code.c_str()),
                wxString::FromUTF8(it->message.c_str())
            );
            if (!it->detail.empty()) {
                text += "  " + wxString::FromUTF8(it->detail.c_str()) + "\n";
            }
            text += wxString::Format("  timestamp: %lld\n\n", static_cast<long long>(it->timestamp));
        }
        body->SetValue(text);
    }

    root->Add(body, 1, wxEXPAND | wxALL, 10);
    auto* buttons = dialog.CreateSeparatedButtonSizer(wxOK);
    root->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    dialog.SetSizerAndFit(root);
    dialog.ShowModal();
}

DashboardFrame::DashboardFrame(wxWindow* parent, DashboardActions actions)
    : wxFrame(parent, wxID_ANY, "EEXI-CII Monitor", wxDefaultPosition, wxSize(560, 670),
          wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT) {
    m_panel = new DashboardPanel(this, std::move(actions));
}

void DashboardFrame::update(
    const PluginSnapshot& snapshot,
    const ProcessStatus status,
    const wxString& message
) {
    m_panel->update(snapshot, status, message);
}

} // namespace eexi_cii
