#include "opencpn/eexi_cii_pi.h"

#include "ui/dashboard_panel.h"
#include "ui/setup_dialog.h"

#include <wx/fileconf.h>
#include <wx/filename.h>

#include <exception>

namespace {

constexpr int kPluginVersionMajor = 0;
constexpr int kPluginVersionMinor = 1;
constexpr int kPluginVersionPatch = 0;
constexpr int kPluginVersionPost = 0;
constexpr const char* kVoyageDataFilename = "voyagedata.json";

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
    return WANTS_NMEA_SENTENCES | WANTS_CONFIG | WANTS_PREFERENCES | WANTS_LATE_INIT;
}

bool eexi_cii_pi::DeInit() {
    save_settings();
    save_accumulator();
    close_dashboard();
    return true;
}

void eexi_cii_pi::LateInit() {
    if (m_setup_required) {
        ShowPreferencesDialog(GetOCPNCanvasWindow());
    } else {
        show_dashboard(GetOCPNCanvasWindow());
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

    try {
        apply_settings(eexi_cii::decode_profile_settings(values));
    } catch (const std::exception& ex) {
        m_settings = eexi_cii::ProfileSettings{};
        m_setup_required = true;
        m_latest_message = wxString::FromUTF8(ex.what());
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
    config->Flush();
}

void eexi_cii_pi::initialize_data_path() {
    wxString* private_data_location = GetpPrivateApplicationDataLocation();
    if (private_data_location == nullptr) {
        return;
    }

    m_data_dir = *private_data_location + "eexi_cii_pi";
    wxFileName::Mkdir(m_data_dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
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
        return;
    }

    if (m_dashboard == nullptr) {
        m_dashboard = new eexi_cii::DashboardFrame(parent);
        m_dashboard->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
            if (event.CanVeto() && m_dashboard != nullptr) {
                m_dashboard->Hide();
                event.Veto();
                return;
            }
            m_dashboard = nullptr;
            event.Skip();
        });
    }

    update_dashboard();
    m_dashboard->Show();
    m_dashboard->Raise();
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

extern "C" DECL_EXP opencpn_plugin* create_pi(void* plugin_manager) {
    return new eexi_cii_pi(plugin_manager);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* plugin) {
    delete plugin;
}
