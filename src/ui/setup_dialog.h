#ifndef EEXI_CII_UI_SETUP_DIALOG_H
#define EEXI_CII_UI_SETUP_DIALOG_H

#include "config/profile_settings.h"

#include <wx/dialog.h>

class wxChoice;
class wxSpinCtrlDouble;
class wxTextCtrl;

namespace eexi_cii {

class SetupDialog final : public wxDialog {
public:
    SetupDialog(wxWindow* parent, const ProfileSettings& initial_settings);

    ProfileSettings settings() const;

private:
    void populate_from_settings(const ProfileSettings& settings);
    void on_ok(wxCommandEvent& event);

    wxTextCtrl* m_name = nullptr;
    wxTextCtrl* m_imo_number = nullptr;
    wxChoice* m_ship_type = nullptr;
    wxChoice* m_fuel_type = nullptr;
    wxSpinCtrlDouble* m_gt = nullptr;
    wxSpinCtrlDouble* m_dwt = nullptr;
    wxSpinCtrlDouble* m_admiralty_coefficient = nullptr;
    wxSpinCtrlDouble* m_sfoc = nullptr;
    wxSpinCtrlDouble* m_displacement = nullptr;
    wxSpinCtrlDouble* m_installed_power_mco = nullptr;
    wxSpinCtrlDouble* m_reference_speed = nullptr;
    wxSpinCtrlDouble* m_sog_threshold = nullptr;
    wxChoice* m_target_year = nullptr;
    wxSpinCtrlDouble* m_ytd_seed_co2 = nullptr;
    wxSpinCtrlDouble* m_ytd_seed_distance = nullptr;
};

} // namespace eexi_cii

#endif
