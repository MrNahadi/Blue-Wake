#include "opencpn/eexi_cii_pi.h"

namespace {

constexpr int kPluginVersionMajor = 0;
constexpr int kPluginVersionMinor = 1;
constexpr int kPluginVersionPatch = 0;
constexpr int kPluginVersionPost = 0;

} // namespace

eexi_cii_pi::eexi_cii_pi(void* plugin_manager)
    : opencpn_plugin_121(plugin_manager),
      m_core(default_profile()) {}

int eexi_cii_pi::Init() {
    m_latest_snapshot = m_core.snapshot();
    m_latest_status = eexi_cii::ProcessStatus::InvalidSentence;
    m_latest_message.clear();
    return WANTS_NMEA_SENTENCES;
}

bool eexi_cii_pi::DeInit() {
    return true;
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

void eexi_cii_pi::SetNMEASentence(wxString& sentence) {
    const auto result = m_core.process_nmea_sentence(sentence.ToStdString());
    m_latest_status = result.status;
    m_latest_snapshot = result.snapshot;
    m_latest_message = wxString::FromUTF8(result.message.c_str());
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

extern "C" DECL_EXP opencpn_plugin* create_pi(void* plugin_manager) {
    return new eexi_cii_pi(plugin_manager);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* plugin) {
    delete plugin;
}
