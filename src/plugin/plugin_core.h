#ifndef EEXI_CII_PLUGIN_PLUGIN_CORE_H
#define EEXI_CII_PLUGIN_PLUGIN_CORE_H

#include "config/profile_settings.h"
#include "data/accumulator.h"
#include "data/nmea_parser.h"
#include "domain/aer_engine.h"
#include "domain/fuel_estimator.h"
#include "domain/vessel_profile.h"

#include <cstdint>
#include <string>
#include <vector>

namespace eexi_cii {

enum class ProcessStatus {
    InvalidSentence,
    BaselineOnly,
    Excluded,
    RejectedOutlier,
    Accumulated,
    Error
};

enum class DiagnosticSeverity {
    Info,
    Warning,
    Error
};

enum class GpsFreshness {
    NoFix,
    Fresh,
    Stale
};

struct DiagnosticEvent {
    DiagnosticSeverity severity = DiagnosticSeverity::Info;
    std::string code;
    std::string message;
    std::int64_t timestamp = 0;
    std::string detail;
};

struct PluginSnapshot {
    VesselProfile profile;
    RMCData last_rmc;
    FuelEstimate last_fuel_estimate;
    YTDState ytd;
    VoyageState current_voyage;
    AERResult aer_result;
    std::vector<DiagnosticEvent> diagnostics;
    GpsFreshness gps_freshness = GpsFreshness::NoFix;
    int seconds_since_last_valid_rmc = -1;
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
    double sog_outlier_limit() const;
    void set_sog_outlier_limit(double limit);

    int target_year_override() const;
    void set_target_year_override(int year);

    void set_ytd_seed(double co2_tonnes, double distance_nm);
    void start_voyage(const std::string& name, double displacement);
    void end_voyage();
    bool check_year_rollover(int current_year);
    const std::vector<VoyageRecord>& voyage_history() const;

    std::string serialize_accumulator_json() const;
    void restore_accumulator_json(const std::string& json);
    void save_accumulator_json(const std::string& filepath) const;
    void load_accumulator_json(const std::string& filepath);

    void record_diagnostic(
        DiagnosticSeverity severity,
        const std::string& code,
        const std::string& message,
        const std::string& detail = ""
    );
    bool poll_gps_freshness();
    const std::vector<DiagnosticEvent>& diagnostic_history() const;

    ProcessResult process_nmea_sentence(const std::string& sentence);
    PluginSnapshot snapshot() const;

private:
    int rating_year(const YTDState& ytd) const;
    PluginSnapshot build_snapshot(bool has_rmc, const RMCData& rmc, const FuelEstimate& estimate) const;
    void mark_valid_rmc_received();

    VesselProfile m_profile;
    Accumulator m_accumulator;
    std::vector<DiagnosticEvent> m_diagnostics;
    double m_sog_threshold = 1.0;
    double m_sog_outlier_limit = 50.0;
    int m_target_year_override = 0;
    std::int64_t m_last_valid_rmc_wall_timestamp = 0;
    int m_invalid_sentence_burst = 0;
    bool m_stale_gps_reported = false;
    RMCData m_last_rmc;
    FuelEstimate m_last_fuel_estimate;
    bool m_has_rmc = false;
};

} // namespace eexi_cii

#endif
