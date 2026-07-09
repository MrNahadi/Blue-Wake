#ifndef EEXI_CII_UI_DASHBOARD_PANEL_H
#define EEXI_CII_UI_DASHBOARD_PANEL_H

#include "plugin/plugin_core.h"

#include <functional>

#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/timer.h>

class wxButton;
class wxStaticText;

namespace eexi_cii {

struct DashboardActions {
    std::function<void(wxWindow*)> open_settings;
    std::function<void(wxWindow*)> export_annual_csv;
    std::function<void(wxWindow*)> export_voyage_csv;
    std::function<void()> refresh;
};

class DashboardPanel final : public wxPanel {
public:
    DashboardPanel(wxWindow* parent, DashboardActions actions);

    void update(const PluginSnapshot& snapshot, ProcessStatus status, const wxString& message);

private:
    void show_diagnostics();

    DashboardActions m_actions;
    wxTimer m_refresh_timer;
    PluginSnapshot m_snapshot;
    bool m_has_snapshot = false;
    wxPanel* m_status_strip = nullptr;
    wxPanel* m_rating_panel = nullptr;
    wxStaticText* m_vessel = nullptr;
    wxStaticText* m_vessel_context = nullptr;
    wxStaticText* m_status = nullptr;
    wxStaticText* m_gps_freshness = nullptr;
    wxStaticText* m_sog = nullptr;
    wxStaticText* m_co2_rate = nullptr;
    wxStaticText* m_ytd_distance = nullptr;
    wxStaticText* m_ytd_co2 = nullptr;
    wxStaticText* m_running_metric_label = nullptr;
    wxStaticText* m_running_aer = nullptr;
    wxStaticText* m_cii_rating = nullptr;
    wxStaticText* m_required_cii = nullptr;
    wxStaticText* m_rating_margin = nullptr;
    wxStaticText* m_voyage_distance = nullptr;
    wxStaticText* m_voyage_co2 = nullptr;
    wxStaticText* m_latest_diagnostic = nullptr;
};

class DashboardFrame final : public wxFrame {
public:
    DashboardFrame(wxWindow* parent, DashboardActions actions);

    void update(const PluginSnapshot& snapshot, ProcessStatus status, const wxString& message);

private:
    DashboardPanel* m_panel = nullptr;
};

} // namespace eexi_cii

#endif
