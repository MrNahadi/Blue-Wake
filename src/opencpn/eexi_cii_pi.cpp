#include "opencpn/eexi_cii_pi.h"

#include "export/csv_exporter.h"
#include "ui/dashboard_panel.h"
#include "ui/setup_dialog.h"

#include <wx/brush.h>
#include <wx/colour.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/pen.h>

#include <exception>
#include <fstream>
#include <string>

namespace {

constexpr int kPluginVersionMajor = 0;
constexpr int kPluginVersionMinor = 1;
constexpr int kPluginVersionPatch = 0;
constexpr int kPluginVersionPost = 0;
constexpr const char* kVoyageDataFilename = "voyagedata.json";
constexpr const char* kDashboardVisibleKey = "DashboardVisible";

wxBitmap make_toolbar_bitmap() {
    wxBitmap bitmap(32, 32);
    wxMemoryDC dc(bitmap);
    dc.SetBackground(wxBrush(wxColour(245, 247, 249)));
    dc.Clear();
    dc.SetPen(wxPen(wxColour(32, 92, 126), 2));
    dc.SetBrush(wxBrush(wxColour(232, 242, 246)));
    dc.DrawRoundedRectangle(3, 5, 26, 22, 3);
    dc.SetPen(wxPen(wxColour(32, 92, 126), 1));
    dc.DrawLine(7, 22, 12, 15);
    dc.DrawLine(12, 15, 17, 19);
    dc.DrawLine(17, 19, 25, 10);
    dc.SelectObject(wxNullBitmap);
    return bitmap;
}

void write_text_file(const wxString& path, const std::string& text) {
    std::ofstream out(path.ToStdString(), std::ios::binary);
    if (!out) {
        throw std::runtime_error("Could not open export file for writing");
    }
    out << text;
    if (!out) {
        throw std::runtime_error("Could not write export file");
    }
}

} // namespace

eexi_cii_pi::eexi_cii_pi(void* plugin_manager)
    : opencpn_plugin_121(plugin_manager),
      m_core(default_profile()) {}

int eexi_cii_pi::Init() {
    m_latest_message.clear();
    load_settings();
    initialize_data_path();
    load_accumulator();

    m_latest_snapshot = m_core.snapshot();
    m_latest_status = eexi_cii::ProcessStatus::InvalidSentence;
    if (m_setup_required && m_latest_message.empty()) {
        m_latest_message = "Vessel profile setup required";
    }
    m_toolbar_bitmap = make_toolbar_bitmap();
    m_toolbar_id = InsertPlugInTool(
        "EEXI-CII",
        &m_toolbar_bitmap,
        &m_toolbar_bitmap,
        wxITEM_CHECK,
        "EEXI/CII Monitor",
        "Show or hide the EEXI-CII monitor",
        nullptr,
        -1,
        -1,
        this
    );
    update_toolbar_state();

    return WANTS_NMEA_SENTENCES | WANTS_CONFIG | WANTS_PREFERENCES | WANTS_LATE_INIT |
           WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL;
}

bool eexi_cii_pi::DeInit() {
    save_settings();
    save_accumulator();
    if (m_toolbar_id != -1) {
        RemovePlugInTool(m_toolbar_id);
        m_toolbar_id = -1;
    }
    close_dashboard();
    return true;
}

void eexi_cii_pi::LateInit() {
    if (m_setup_required) {
        ShowPreferencesDialog(GetOCPNCanvasWindow());
    } else if (m_dashboard_visible) {
        show_dashboard(GetOCPNCanvasWindow());
    } else {
        update_toolbar_state();
    }
}

int eexi_cii_pi::GetAPIVersionMajor() {
    return API_VERSION_MAJOR;
}

int eexi_cii_pi::GetAPIVersionMinor() {
    return API_VERSION_MINOR;
}

int eexi_cii_pi::GetPlugInVersionMajor() {
    return kPluginVersionMajor;
}

int eexi_cii_pi::GetPlugInVersionMinor() {
    return kPluginVersionMinor;
}

int eexi_cii_pi::GetPlugInVersionPatch() {
    return kPluginVersionPatch;
}

int eexi_cii_pi::GetPlugInVersionPost() {
    return kPluginVersionPost;
}

const char* eexi_cii_pi::GetPlugInVersionPre() {
    return "";
}

const char* eexi_cii_pi::GetPlugInVersionBuild() {
    return "";
}

int eexi_cii_pi::GetToolbarToolCount() {
    return 1;
}

wxString eexi_cii_pi::GetCommonName() {
    return "EEXI-CII";
}

wxString eexi_cii_pi::GetShortDescription() {
    return "EEXI and CII monitoring";
}

wxString eexi_cii_pi::GetLongDescription() {
    return "Estimates fuel use from own-ship RMC sentences and tracks CII/AER progress for configured vessels.";
}

void eexi_cii_pi::ShowPreferencesDialog(wxWindow* parent) {
    eexi_cii::SetupDialog dialog(parent, m_settings);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    try {
        apply_settings(dialog.settings());
        save_settings();
        show_dashboard(parent);
    } catch (const std::exception& ex) {
        m_setup_required = true;
        m_latest_status = eexi_cii::ProcessStatus::Error;
        m_latest_message = wxString::FromUTF8(ex.what());
        record_diagnostic(
            eexi_cii::DiagnosticSeverity::Error,
            "settings_apply_failed",
            "Vessel profile settings could not be applied",
            ex.what()
        );
    }
}

void eexi_cii_pi::OnToolbarToolCallback(int id) {
    if (id != m_toolbar_id) {
        return;
    }

    if (m_dashboard != nullptr && m_dashboard->IsShown()) {
        hide_dashboard();
    } else {
        show_dashboard(GetOCPNCanvasWindow());
    }
}

void eexi_cii_pi::SetNMEASentence(wxString& sentence) {
    if (m_setup_required) {
        m_latest_status = eexi_cii::ProcessStatus::Error;
        m_latest_snapshot = m_core.snapshot();
        m_latest_message = "Vessel profile setup required";
        return;
    }

    const auto result = m_core.process_nmea_sentence(sentence.ToStdString());
    m_latest_status = result.status;
    m_latest_snapshot = result.snapshot;
    m_latest_message = wxString::FromUTF8(result.message.c_str());
    update_dashboard();
}

const eexi_cii::PluginSnapshot& eexi_cii_pi::latest_snapshot() const {
    return m_latest_snapshot;
}

eexi_cii::ProcessStatus eexi_cii_pi::latest_status() const {
    return m_latest_status;
}

const wxString& eexi_cii_pi::latest_message() const {
    return m_latest_message;
}

bool eexi_cii_pi::setup_required() const {
    return m_setup_required;
}

eexi_cii::VesselProfile eexi_cii_pi::default_profile() {
    eexi_cii::VesselProfile profile;
    profile.name = "Configure vessel profile";
    profile.ship_type = eexi_cii::ShipType::BulkCarrier;
    profile.dwt = 50000.0;
    profile.gt = 30000.0;
    profile.admiralty_coefficient = 500.0;
    profile.sfoc = 190.0;
    profile.fuel_type = eexi_cii::FuelType::Hfo;
    profile.displacement = 60000.0;
    return profile;
}

wxString eexi_cii_pi::config_path() {
    return "/PlugIns/EEXI-CII";
}

void eexi_cii_pi::load_settings() {
    wxFileConfig* config = GetOCPNConfigObject();
    if (config == nullptr) {
        m_settings = eexi_cii::ProfileSettings{};
        m_setup_required = true;
        record_diagnostic(
            eexi_cii::DiagnosticSeverity::Error,
            "config_unavailable",
            "OpenCPN configuration object is unavailable"
        );
        return;
    }

    eexi_cii::SettingsMap values;
    const eexi_cii::SettingsMap keys = eexi_cii::encode_profile_settings(eexi_cii::ProfileSettings{});

    config->SetPath(config_path());
    for (const auto& item : keys) {
        wxString value;
        if (config->Read(wxString::FromUTF8(item.first.c_str()), &value)) {
            values[item.first] = value.ToStdString();
        }
    }
    config->Read(kDashboardVisibleKey, &m_dashboard_visible, true);

    try {
        apply_settings(eexi_cii::decode_profile_settings(values));
    } catch (const std::exception& ex) {
        m_settings = eexi_cii::ProfileSettings{};
        m_setup_required = true;
        m_latest_message = wxString::FromUTF8(ex.what());
        record_diagnostic(
            eexi_cii::DiagnosticSeverity::Error,
            "settings_load_failed",
            "Stored vessel profile settings could not be loaded",
            ex.what()
        );
    }
}

void eexi_cii_pi::save_settings() {
    wxFileConfig* config = GetOCPNConfigObject();
    if (config == nullptr) {
        return;
    }

    config->SetPath(config_path());
    const eexi_cii::SettingsMap values = eexi_cii::encode_profile_settings(m_settings);
    for (const auto& item : values) {
        config->Write(
            wxString::FromUTF8(item.first.c_str()),
            wxString::FromUTF8(item.second.c_str())
        );
    }
    config->Write(kDashboardVisibleKey, m_dashboard_visible);
    config->Flush();
}

void eexi_cii_pi::initialize_data_path() {
    wxString* private_data_location = GetpPrivateApplicationDataLocation();
    if (private_data_location == nullptr) {
        record_diagnostic(
            eexi_cii::DiagnosticSeverity::Error,
            "data_path_unavailable",
            "OpenCPN private plugin data path is unavailable"
        );
        return;
    }

    m_data_dir = *private_data_location + "eexi_cii_pi";
    if (!wxFileName::Mkdir(m_data_dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL) &&
        !wxFileName::DirExists(m_data_dir)) {
        record_diagnostic(
            eexi_cii::DiagnosticSeverity::Error,
            "data_path_create_failed",
            "Could not create plugin data directory",
            m_data_dir.ToStdString()
        );
        return;
    }
    m_json_path = m_data_dir + wxFileName::GetPathSeparator() + kVoyageDataFilename;
}

void eexi_cii_pi::load_accumulator() {
    if (m_json_path.empty() || !wxFileName::FileExists(m_json_path)) {
        return;
    }

    try {
        m_core.load_accumulator_json(m_json_path.ToStdString());
    } catch (const std::exception& ex) {
        m_latest_message = wxString::FromUTF8(ex.what());
        record_diagnostic(
            eexi_cii::DiagnosticSeverity::Error,
            "persistence_load_failed",
            "Accumulator state could not be loaded",
            ex.what()
        );
    }
}

void eexi_cii_pi::save_accumulator() {
    if (m_json_path.empty()) {
        return;
    }

    try {
        m_core.save_accumulator_json(m_json_path.ToStdString());
    } catch (const std::exception& ex) {
        m_latest_message = wxString::FromUTF8(ex.what());
        record_diagnostic(
            eexi_cii::DiagnosticSeverity::Error,
            "persistence_save_failed",
            "Accumulator state could not be saved",
            ex.what()
        );
    }
}

void eexi_cii_pi::apply_settings(const eexi_cii::ProfileSettings& settings) {
    m_settings = settings;
    m_setup_required = m_settings.setup_required();
    if (m_setup_required) {
        return;
    }

    m_core.apply_settings(m_settings);
    m_latest_snapshot = m_core.snapshot();
    m_latest_status = eexi_cii::ProcessStatus::InvalidSentence;
    m_latest_message.clear();
    update_dashboard();
}

void eexi_cii_pi::show_dashboard(wxWindow* parent) {
    if (m_setup_required) {
        m_dashboard_visible = false;
        save_settings();
        update_toolbar_state();
        return;
    }

    if (m_dashboard == nullptr) {
        eexi_cii::DashboardActions actions;
        actions.open_settings = [this](wxWindow* window) { ShowPreferencesDialog(window); };
        actions.export_annual_csv = [this](wxWindow* window) { export_annual_csv(window); };
        actions.export_voyage_csv = [this](wxWindow* window) { export_voyage_csv(window); };
        actions.refresh = [this]() {
            if (!m_dashboard_visible) {
                return;
            }
            const bool stale_logged = m_core.poll_gps_freshness();
            m_latest_snapshot = m_core.snapshot();
            if (stale_logged) {
                m_latest_status = eexi_cii::ProcessStatus::Error;
                m_latest_message = "No valid RMC data received for over 60 seconds";
                wxLogWarning("%s", m_latest_message.c_str());
            }
            update_dashboard();
        };

        m_dashboard = new eexi_cii::DashboardFrame(parent, actions);
        m_dashboard->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
            if (event.CanVeto() && m_dashboard != nullptr) {
                hide_dashboard();
                event.Veto();
                return;
            }
            m_dashboard = nullptr;
            event.Skip();
        });
    }

    m_dashboard_visible = true;
    update_dashboard();
    m_dashboard->Show();
    m_dashboard->Raise();
    update_toolbar_state();
    save_settings();
}

void eexi_cii_pi::record_diagnostic(
    const eexi_cii::DiagnosticSeverity severity,
    const std::string& code,
    const std::string& message,
    const std::string& detail
) {
    m_core.record_diagnostic(severity, code, message, detail);
    m_latest_snapshot = m_core.snapshot();
    m_latest_message = wxString::FromUTF8(message.c_str());

    const wxString log_message =
        wxString::FromUTF8(code.c_str()) + ": " + wxString::FromUTF8(message.c_str());
    switch (severity) {
    case eexi_cii::DiagnosticSeverity::Info:
        wxLogMessage("%s", log_message.c_str());
        break;
    case eexi_cii::DiagnosticSeverity::Warning:
        wxLogWarning("%s", log_message.c_str());
        break;
    case eexi_cii::DiagnosticSeverity::Error:
        wxLogError("%s", log_message.c_str());
        break;
    }
    update_dashboard();
}

void eexi_cii_pi::export_annual_csv(wxWindow* parent) {
    m_latest_snapshot = m_core.snapshot();
    if (!m_latest_snapshot.has_aer) {
        wxMessageBox(
            "No annual CII summary is available until distance and CO2 have accumulated.",
            "EEXI-CII Export",
            wxOK | wxICON_INFORMATION,
            parent
        );
        return;
    }

    wxFileDialog dialog(
        parent,
        "Export annual CII summary",
        "",
        "eexi_cii_annual_summary.csv",
        "CSV files (*.csv)|*.csv",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT
    );
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    try {
        write_text_file(
            dialog.GetPath(),
            eexi_cii::export_annual_csv(
                m_latest_snapshot.ytd,
                m_core.profile(),
                m_latest_snapshot.aer_result
            )
        );
        record_diagnostic(
            eexi_cii::DiagnosticSeverity::Info,
            "annual_csv_exported",
            "Annual CSV summary exported",
            dialog.GetPath().ToStdString()
        );
    } catch (const std::exception& ex) {
        record_diagnostic(
            eexi_cii::DiagnosticSeverity::Error,
            "annual_csv_export_failed",
            "Annual CSV summary could not be exported",
            ex.what()
        );
        wxMessageBox(wxString::FromUTF8(ex.what()), "EEXI-CII Export", wxOK | wxICON_ERROR, parent);
    }
}

void eexi_cii_pi::export_voyage_csv(wxWindow* parent) {
    if (m_core.voyage_history().empty()) {
        wxMessageBox(
            "No completed voyage records are available to export.",
            "EEXI-CII Export",
            wxOK | wxICON_INFORMATION,
            parent
        );
        return;
    }

    wxFileDialog dialog(
        parent,
        "Export voyage breakdown",
        "",
        "eexi_cii_voyages.csv",
        "CSV files (*.csv)|*.csv",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT
    );
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    try {
        write_text_file(dialog.GetPath(), eexi_cii::export_voyage_csv(m_core.voyage_history()));
        record_diagnostic(
            eexi_cii::DiagnosticSeverity::Info,
            "voyage_csv_exported",
            "Voyage CSV breakdown exported",
            dialog.GetPath().ToStdString()
        );
    } catch (const std::exception& ex) {
        record_diagnostic(
            eexi_cii::DiagnosticSeverity::Error,
            "voyage_csv_export_failed",
            "Voyage CSV breakdown could not be exported",
            ex.what()
        );
        wxMessageBox(wxString::FromUTF8(ex.what()), "EEXI-CII Export", wxOK | wxICON_ERROR, parent);
    }
}

void eexi_cii_pi::hide_dashboard() {
    if (m_dashboard != nullptr) {
        m_dashboard->Hide();
    }
    m_dashboard_visible = false;
    update_toolbar_state();
    save_settings();
}

void eexi_cii_pi::update_dashboard() {
    if (m_dashboard != nullptr) {
        m_dashboard->update(m_latest_snapshot, m_latest_status, m_latest_message);
    }
}

void eexi_cii_pi::close_dashboard() {
    if (m_dashboard != nullptr) {
        m_dashboard->Destroy();
        m_dashboard = nullptr;
    }
}

void eexi_cii_pi::update_toolbar_state() {
    if (m_toolbar_id != -1) {
        SetToolbarItemState(m_toolbar_id, m_dashboard_visible && !m_setup_required);
    }
}

extern "C" EEXI_CII_PLUGIN_EXPORT opencpn_plugin* create_pi(void* plugin_manager) {
    return new eexi_cii_pi(plugin_manager);
}

extern "C" EEXI_CII_PLUGIN_EXPORT void destroy_pi(opencpn_plugin* plugin) {
    delete plugin;
}
