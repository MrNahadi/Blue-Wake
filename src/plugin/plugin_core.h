#ifndef EEXI_CII_PLUGIN_PLUGIN_CORE_H
#define EEXI_CII_PLUGIN_PLUGIN_CORE_H

#include "config/profile_settings.h"
#include "data/accumulator.h"
#include "data/nmea_parser.h"
#include "domain/aer_engine.h"
#include "domain/fuel_estimator.h"
#include "domain/vessel_profile.h"

#include <string>

namespace eexi_cii {

enum class ProcessStatus {
    InvalidSentence,
    BaselineOnly,
    Excluded,
    Accumulated,
    Error
};

struct PluginSnapshot {
    RMCData last_rmc;
    FuelEstimate last_fuel_estimate;
    YTDState ytd;
    VoyageState current_voyage;
    AERResult aer_result;
    bool has_rmc = false;
    bool has_aer = false;
};

struct ProcessResult {
    ProcessStatus status = ProcessStatus::InvalidSentence;
    std::string message;
    PluginSnapshot snapshot;
};

class PluginCore {
public:
    explicit PluginCore(VesselProfile profile);

    const VesselProfile& profile() const;
    void set_profile(const VesselProfile& profile);
    void apply_settings(const ProfileSettings& settings);

    double sog_threshold() const;
    void set_sog_threshold(double threshold);

    int target_year_override() const;
    void set_target_year_override(int year);

    void set_ytd_seed(double co2_tonnes, double distance_nm);
    void start_voyage(const std::string& name, double displacement);
    void end_voyage();

    ProcessResult process_nmea_sentence(const std::string& sentence);
    PluginSnapshot snapshot() const;

private:
    int rating_year(const YTDState& ytd) const;
    PluginSnapshot build_snapshot(bool has_rmc, const RMCData& rmc, const FuelEstimate& estimate) const;

    VesselProfile m_profile;
    Accumulator m_accumulator;
    double m_sog_threshold = 1.0;
    int m_target_year_override = 0;
    RMCData m_last_rmc;
    FuelEstimate m_last_fuel_estimate;
    bool m_has_rmc = false;
};

} // namespace eexi_cii

#endif
