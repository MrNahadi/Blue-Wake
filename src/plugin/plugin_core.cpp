#include "plugin/plugin_core.h"

#include <stdexcept>
#include <utility>

namespace eexi_cii {

PluginCore::PluginCore(VesselProfile profile)
    : m_profile(std::move(profile)) {}

const VesselProfile& PluginCore::profile() const {
    return m_profile;
}

void PluginCore::set_profile(const VesselProfile& profile) {
    m_profile = profile;
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

int PluginCore::target_year_override() const {
    return m_target_year_override;
}

void PluginCore::set_target_year_override(const int year) {
    if (year != 0 && year < 2023) {
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

ProcessResult PluginCore::process_nmea_sentence(const std::string& sentence) {
    ProcessResult result;

    const RMCData rmc = parse_rmc(sentence);
    if (!rmc.valid) {
        result.status = ProcessStatus::InvalidSentence;
        result.message = "Invalid or unsupported RMC sentence";
        result.snapshot = snapshot();
        return result;
    }

    try {
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
    value.last_rmc = rmc;
    value.last_fuel_estimate = estimate;
    value.ytd = m_accumulator.year_to_date();
    value.current_voyage = m_accumulator.current_voyage();
    value.has_rmc = has_rmc;

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

} // namespace eexi_cii
