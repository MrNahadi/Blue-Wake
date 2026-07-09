#include "ui/dashboard_panel.h"

#include <wx/colour.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <iomanip>
#include <sstream>

namespace eexi_cii {
namespace {

wxStaticText* add_value(wxWindow* parent, wxFlexGridSizer* grid, const wxString& label) {
    grid->Add(new wxStaticText(parent, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    auto* value = new wxStaticText(parent, wxID_ANY, "--");
    grid->Add(value, 1, wxEXPAND | wxALL, 4);
    return value;
}

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
        return "Below SOG threshold";
    case ProcessStatus::RejectedOutlier:
        return "SOG outlier rejected";
    case ProcessStatus::Accumulated:
        return "Accumulating";
    case ProcessStatus::Error:
        return "Error";
    }

    return "Unknown";
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

} // namespace

DashboardPanel::DashboardPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    auto* root = new wxBoxSizer(wxVERTICAL);
    auto* title = new wxStaticText(this, wxID_ANY, "EEXI-CII Monitor");
    wxFont title_font = title->GetFont();
    title_font.SetPointSize(title_font.GetPointSize() + 2);
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(title_font);
    root->Add(title, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    auto* grid = new wxFlexGridSizer(2, 8, 10);
    grid->AddGrowableCol(1, 1);
    m_status = add_value(this, grid, "Status");
    m_sog = add_value(this, grid, "SOG");
    m_co2_rate = add_value(this, grid, "CO2 rate");
    m_ytd_distance = add_value(this, grid, "YTD distance");
    m_ytd_co2 = add_value(this, grid, "YTD CO2");
    m_running_aer = add_value(this, grid, "Running AER");
    m_cii_rating = add_value(this, grid, "CII rating");
    m_required_cii = add_value(this, grid, "Required CII");
    m_voyage_distance = add_value(this, grid, "Voyage distance");
    m_voyage_co2 = add_value(this, grid, "Voyage CO2");
    root->Add(grid, 1, wxEXPAND | wxALL, 12);
    SetSizer(root);
}

void DashboardPanel::update(
    const PluginSnapshot& snapshot,
    const ProcessStatus status,
    const wxString& message
) {
    m_status->SetLabel(status_text(status, message));

    if (snapshot.has_rmc) {
        m_sog->SetLabel(number(snapshot.last_rmc.sog_knots, 2) + " kn");
        m_co2_rate->SetLabel(number(snapshot.last_fuel_estimate.co2_rate_tonnes_hr, 3) + " t/hr");
    } else {
        m_sog->SetLabel("--");
        m_co2_rate->SetLabel("--");
    }

    m_ytd_distance->SetLabel(number(snapshot.ytd.distance_nm, 2) + " nm");
    m_ytd_co2->SetLabel(number(snapshot.ytd.co2_tonnes, 3) + " t");

    if (snapshot.has_aer) {
        m_running_aer->SetLabel(number(snapshot.aer_result.aer, 4));
        m_cii_rating->SetLabel(wxString::Format("%c", snapshot.aer_result.rating));
        m_cii_rating->SetForegroundColour(rating_colour(snapshot.aer_result.rating));
        m_required_cii->SetLabel(number(snapshot.aer_result.required_cii, 4));
    } else {
        m_running_aer->SetLabel("--");
        m_cii_rating->SetLabel("--");
        m_cii_rating->SetForegroundColour(rating_colour('\0'));
        m_required_cii->SetLabel("--");
    }

    if (snapshot.current_voyage.active) {
        m_voyage_distance->SetLabel(number(snapshot.current_voyage.distance_nm, 2) + " nm");
        m_voyage_co2->SetLabel(number(snapshot.current_voyage.co2_tonnes, 3) + " t");
    } else {
        m_voyage_distance->SetLabel("--");
        m_voyage_co2->SetLabel("--");
    }

    Layout();
}

DashboardFrame::DashboardFrame(wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "EEXI-CII Monitor", wxDefaultPosition, wxSize(420, 390),
          wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT) {
    m_panel = new DashboardPanel(this);
}

void DashboardFrame::update(
    const PluginSnapshot& snapshot,
    const ProcessStatus status,
    const wxString& message
) {
    m_panel->update(snapshot, status, message);
}

} // namespace eexi_cii
