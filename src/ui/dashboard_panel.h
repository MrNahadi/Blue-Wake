#ifndef EEXI_CII_UI_DASHBOARD_PANEL_H
#define EEXI_CII_UI_DASHBOARD_PANEL_H

#include "plugin/plugin_core.h"

#include <wx/frame.h>
#include <wx/panel.h>

class wxStaticText;

namespace eexi_cii {

class DashboardPanel final : public wxPanel {
public:
    explicit DashboardPanel(wxWindow* parent);

    void update(const PluginSnapshot& snapshot, ProcessStatus status, const wxString& message);

private:
    wxStaticText* m_status = nullptr;
    wxStaticText* m_sog = nullptr;
    wxStaticText* m_co2_rate = nullptr;
    wxStaticText* m_ytd_distance = nullptr;
    wxStaticText* m_ytd_co2 = nullptr;
    wxStaticText* m_running_aer = nullptr;
    wxStaticText* m_cii_rating = nullptr;
    wxStaticText* m_required_cii = nullptr;
    wxStaticText* m_voyage_distance = nullptr;
    wxStaticText* m_voyage_co2 = nullptr;
};

class DashboardFrame final : public wxFrame {
public:
    explicit DashboardFrame(wxWindow* parent);

    void update(const PluginSnapshot& snapshot, ProcessStatus status, const wxString& message);

private:
    DashboardPanel* m_panel = nullptr;
};

} // namespace eexi_cii

#endif
