#ifndef EEXI_CII_OPENCPN_EEXI_CII_PI_H
#define EEXI_CII_OPENCPN_EEXI_CII_PI_H

#include "plugin/plugin_core.h"

#include "ocpn_plugin.h"

class eexi_cii_pi final : public opencpn_plugin_121 {
public:
    explicit eexi_cii_pi(void* plugin_manager);

    int Init() override;
    bool DeInit() override;
    void LateInit() override;

    int GetAPIVersionMajor() override;
    int GetAPIVersionMinor() override;
    int GetPlugInVersionMajor() override;
    int GetPlugInVersionMinor() override;
    int GetPlugInVersionPatch() override;
    int GetPlugInVersionPost() override;
    const char* GetPlugInVersionPre() override;
    const char* GetPlugInVersionBuild() override;

    wxString GetCommonName() override;
    wxString GetShortDescription() override;
    wxString GetLongDescription() override;
    void ShowPreferencesDialog(wxWindow* parent) override;

    void SetNMEASentence(wxString& sentence) override;

    const eexi_cii::PluginSnapshot& latest_snapshot() const;
    eexi_cii::ProcessStatus latest_status() const;
    const wxString& latest_message() const;
    bool setup_required() const;

private:
    static eexi_cii::VesselProfile default_profile();
    static wxString config_path();

    void load_settings();
    void save_settings();
    void initialize_data_path();
    void load_accumulator();
    void save_accumulator();
    void apply_settings(const eexi_cii::ProfileSettings& settings);

    eexi_cii::PluginCore m_core;
    eexi_cii::ProfileSettings m_settings;
    eexi_cii::PluginSnapshot m_latest_snapshot;
    eexi_cii::ProcessStatus m_latest_status = eexi_cii::ProcessStatus::InvalidSentence;
    wxString m_latest_message;
    wxString m_data_dir;
    wxString m_json_path;
    bool m_setup_required = true;
};

extern "C" DECL_EXP opencpn_plugin* create_pi(void* plugin_manager);
extern "C" DECL_EXP void destroy_pi(opencpn_plugin* plugin);

#endif
