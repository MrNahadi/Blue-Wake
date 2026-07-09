#include "plugin/plugin_core.h"

#include "data/persistence.h"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace eexi_cii {
namespace {

constexpr std::size_t kMaxDiagnostics = 100;
constexpr std::int64_t kGpsStaleSeconds = 60;

std::int64_t current_unix_seconds() {
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

} // namespace

PluginCore::PluginCore(VesselProfile profile)
    : m_profile(std::move(profile)) {}

const VesselProfile& PluginCore::profile() const {
    return m_profile;
}

void PluginCore::set_profile(const VesselProfile& profile) {
    m_profile = profile;
}

void PluginCore::apply_settings(const ProfileSettings& settings) {
    if (settings.setup_required()) {
        record_diagnostic(
            DiagnosticSeverity::Error,
            "profile_incomplete",
            "Vessel profile setup is incomplete",
            "Required vessel profile fields were missing or invalid."
        );
        throw std::invalid_argument("Vessel profile setup is required");
    }

    set_profile(settings.profile);
    set_sog_threshold(settings.sog_threshold);
    set_target_year_override(settings.target_year_override);
    set_ytd_seed(settings.ytd_seed_co2_tonnes, settings.ytd_seed_distance_nm);
}

double PluginCore::sog_threshold() const {
    return m_sog_threshold;
}

void PluginCore::set_sog_threshold(const double threshold) {
    if (threshold < 0.0) {
        throw std::invalid_argument("SOG threshold cannot be negative");
    }

    m_sog_threshold = threshold;
}

double PluginCore::sog_outlier_limit() const {
    return m_sog_outlier_limit;
}

void PluginCore::set_sog_outlier_limit(const double limit) {
    if (limit <= 0.0) {
        throw std::invalid_argument("SOG outlier limit must be greater than zero");
    }

    m_sog_outlier_limit = limit;
}

int PluginCore::target_year_override() const {
    return m_target_year_override;
}

void PluginCore::set_target_year_override(const int year) {
    if (year != 0 && year < 2023) {
        record_diagnostic(
            DiagnosticSeverity::Error,
            "target_year_invalid",
            "Target year override was rejected",
            "The target year cannot be before 2023."
        );
        throw std::invalid_argument("Target year override cannot be before 2023");
    }

    m_target_year_override = year;
}

void PluginCore::set_ytd_seed(const double co2_tonnes, const double distance_nm) {
    m_accumulator.set_ytd_seed(co2_tonnes, distance_nm);
}

void PluginCore::start_voyage(const std::string& name, const double displacement) {
    m_accumulator.start_voyage(name, displacement);
}

void PluginCore::end_voyage() {
    const YTDState ytd = m_accumulator.year_to_date();
    m_accumulator.end_voyage(m_profile, rating_year(ytd));
}

bool PluginCore::check_year_rollover(const int current_year) {
    const bool rolled = m_accumulator.check_year_rollover(current_year, m_profile);
    if (rolled) {
        record_diagnostic(
            DiagnosticSeverity::Info,
            "year_rollover",
            "Year-to-date CII state was archived",
            "A new calendar year was detected from incoming RMC data."
        );
    }
    return rolled;
}

const std::vector<VoyageRecord>& PluginCore::voyage_history() const {
    return m_accumulator.voyage_history();
}

std::string PluginCore::serialize_accumulator_json() const {
    return serialize_accumulator(m_accumulator);
}

void PluginCore::restore_accumulator_json(const std::string& json) {
    m_accumulator = deserialize_accumulator(json);
}

void PluginCore::save_accumulator_json(const std::string& filepath) const {
    save_accumulator(filepath, m_accumulator);
}

void PluginCore::load_accumulator_json(const std::string& filepath) {
    m_accumulator = load_accumulator(filepath);
}

void PluginCore::record_diagnostic(
    const DiagnosticSeverity severity,
    const std::string& code,
    const std::string& message,
    const std::string& detail
) {
    if (m_diagnostics.size() >= kMaxDiagnostics) {
        m_diagnostics.erase(m_diagnostics.begin());
    }

    m_diagnostics.push_back(DiagnosticEvent{
        severity,
        code,
        message,
        current_unix_seconds(),
        detail
    });
}

bool PluginCore::poll_gps_freshness() {
    if (m_last_valid_rmc_wall_timestamp == 0) {
        return false;
    }

    const std::int64_t elapsed = current_unix_seconds() - m_last_valid_rmc_wall_timestamp;
    if (elapsed <= kGpsStaleSeconds || m_stale_gps_reported) {
        return false;
    }

    record_diagnostic(
        DiagnosticSeverity::Warning,
        "gps_stale",
        "No valid RMC data received for over 60 seconds",
        "The accumulator remains frozen until valid own-ship RMC data resumes."
    );
    m_stale_gps_reported = true;
    return true;
}

const std::vector<DiagnosticEvent>& PluginCore::diagnostic_history() const {
    return m_diagnostics;
}

ProcessResult PluginCore::process_nmea_sentence(const std::string& sentence) {
    ProcessResult result;

    const RMCData rmc = parse_rmc(sentence);
    if (!rmc.valid) {
        ++m_invalid_sentence_burst;
        if (m_invalid_sentence_burst == 1 || m_invalid_sentence_burst % 10 == 0) {
            record_diagnostic(
                DiagnosticSeverity::Warning,
                "invalid_rmc_burst",
                "Invalid or unsupported RMC sentence received",
                sentence
            );
        }
        result.status = ProcessStatus::InvalidSentence;
        result.message = "Invalid or unsupported RMC sentence";
        result.snapshot = snapshot();
        return result;
    }

    mark_valid_rmc_received();
    m_invalid_sentence_burst = 0;

    if (rmc.sog_knots > m_sog_outlier_limit) {
        record_diagnostic(
            DiagnosticSeverity::Warning,
            "sog_outlier",
            "Data point rejected above SOG outlier limit",
            "SOG=" + std::to_string(rmc.sog_knots) + " kn"
        );
        result.status = ProcessStatus::RejectedOutlier;
        result.message = "Data point rejected above SOG outlier limit";
        result.snapshot = snapshot();
        return result;
    }

    try {
        check_year_rollover(rmc.year);

        const FuelEstimate estimate = estimate_fuel(rmc.sog_knots, m_profile, m_sog_threshold);
        const bool accumulated = m_accumulator.add_data_point(rmc, estimate, m_sog_threshold);

        m_last_rmc = rmc;
        m_last_fuel_estimate = estimate;
        m_has_rmc = true;

        if (accumulated) {
            result.status = ProcessStatus::Accumulated;
            result.message = "Data point accumulated";
        } else if (estimate.below_threshold || rmc.sog_knots < m_sog_threshold) {
            result.status = ProcessStatus::Excluded;
            result.message = "Data point excluded below SOG threshold";
        } else {
            result.status = ProcessStatus::BaselineOnly;
            result.message = "Data point stored as baseline";
        }

        result.snapshot = build_snapshot(true, rmc, estimate);
        return result;
    } catch (const std::exception& ex) {
        record_diagnostic(
            DiagnosticSeverity::Error,
            "processing_error",
            "RMC sentence could not be processed",
            ex.what()
        );
        result.status = ProcessStatus::Error;
        result.message = ex.what();
        result.snapshot = snapshot();
        return result;
    }
}

PluginSnapshot PluginCore::snapshot() const {
    return build_snapshot(m_has_rmc, m_last_rmc, m_last_fuel_estimate);
}

int PluginCore::rating_year(const YTDState& ytd) const {
    if (m_target_year_override != 0) {
        return m_target_year_override;
    }
    if (ytd.year != 0) {
        return ytd.year;
    }
    return 0;
}

PluginSnapshot PluginCore::build_snapshot(
    const bool has_rmc,
    const RMCData& rmc,
    const FuelEstimate& estimate
) const {
    PluginSnapshot value;
    value.profile = m_profile;
    value.last_rmc = rmc;
    value.last_fuel_estimate = estimate;
    value.ytd = m_accumulator.year_to_date();
    value.current_voyage = m_accumulator.current_voyage();
    value.has_rmc = has_rmc;
    value.diagnostics = m_diagnostics;

    if (!has_rmc || m_last_valid_rmc_wall_timestamp == 0) {
        value.gps_freshness = GpsFreshness::NoFix;
        value.seconds_since_last_valid_rmc = -1;
    } else {
        const std::int64_t elapsed = current_unix_seconds() - m_last_valid_rmc_wall_timestamp;
        value.seconds_since_last_valid_rmc = elapsed > 2147483647 ? 2147483647 : static_cast<int>(elapsed);
        value.gps_freshness =
            elapsed > kGpsStaleSeconds ? GpsFreshness::Stale : GpsFreshness::Fresh;
    }

    if (value.ytd.distance_nm > 0.0) {
        try {
            value.aer_result = compute_aer(
                value.ytd.co2_tonnes,
                value.ytd.distance_nm,
                m_profile,
                rating_year(value.ytd)
            );
            value.has_aer = true;
        } catch (const std::exception&) {
            value.has_aer = false;
        }
    }

    return value;
}

void PluginCore::mark_valid_rmc_received() {
    m_last_valid_rmc_wall_timestamp = current_unix_seconds();
    m_stale_gps_reported = false;
}

} // namespace eexi_cii
