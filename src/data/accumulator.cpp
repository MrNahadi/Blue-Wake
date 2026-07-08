#include "data/accumulator.h"

#include "data/haversine.h"

#include <stdexcept>

namespace eexi_cii {
namespace {

std::int64_t days_from_civil(const int year, const unsigned month, const unsigned day) {
    const int adjusted_year = year - (month <= 2 ? 1 : 0);
    const int era = (adjusted_year >= 0 ? adjusted_year : adjusted_year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(adjusted_year - era * 400);
    const unsigned doy =
        (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int>(doe) - 719468;
}

void add_to_voyage(VoyageState& voyage, const double distance_nm, const double co2_tonnes,
                   const double elapsed_seconds) {
    voyage.distance_nm += distance_nm;
    voyage.co2_tonnes += co2_tonnes;
    voyage.elapsed_seconds += elapsed_seconds;
}

} // namespace

std::int64_t rmc_timestamp_seconds(const RMCData& rmc) {
    if (!rmc.valid || rmc.year <= 0 || rmc.month < 1 || rmc.month > 12 ||
        rmc.day < 1 || rmc.day > 31 || rmc.utc_seconds < 0.0) {
        throw std::invalid_argument("RMC timestamp is invalid");
    }

    const std::int64_t days =
        days_from_civil(rmc.year, static_cast<unsigned>(rmc.month), static_cast<unsigned>(rmc.day));
    return days * 86400 + static_cast<std::int64_t>(rmc.utc_seconds);
}

void Accumulator::start_voyage(const std::string& name, const double displacement) {
    if (name.empty()) {
        throw std::invalid_argument("Voyage name cannot be empty");
    }
    if (displacement <= 0.0) {
        throw std::invalid_argument("Voyage displacement must be greater than zero");
    }
    if (m_current_voyage.active) {
        throw std::logic_error("A voyage is already active");
    }

    m_current_voyage = VoyageState{};
    m_current_voyage.name = name;
    m_current_voyage.displacement = displacement;
    m_current_voyage.active = true;
    m_current_voyage.start_timestamp = m_has_last_fix ? m_last_timestamp : 0;
}

void Accumulator::end_voyage(const VesselProfile& profile, const int year) {
    if (!m_current_voyage.active) {
        throw std::logic_error("No active voyage to end");
    }

    double voyage_aer = 0.0;
    if (m_current_voyage.distance_nm > 0.0 && m_current_voyage.co2_tonnes >= 0.0) {
        voyage_aer = compute_aer(
                         m_current_voyage.co2_tonnes,
                         m_current_voyage.distance_nm,
                         profile,
                         year
                     )
                         .aer;
    }

    m_history.push_back(VoyageRecord{
        m_current_voyage.name,
        m_current_voyage.start_timestamp,
        m_has_last_fix ? m_last_timestamp : 0,
        m_current_voyage.displacement,
        m_current_voyage.distance_nm,
        m_current_voyage.co2_tonnes,
        voyage_aer,
        false
    });

    m_current_voyage = VoyageState{};
}

bool Accumulator::has_active_voyage() const {
    return m_current_voyage.active;
}

bool Accumulator::add_data_point(
    const RMCData& rmc,
    const FuelEstimate& estimate,
    const double sog_threshold
) {
    if (!rmc.valid || rmc.sog_knots < 0.0) {
        return false;
    }
    if (sog_threshold < 0.0) {
        throw std::invalid_argument("SOG threshold cannot be negative");
    }

    const std::int64_t timestamp = rmc_timestamp_seconds(rmc);
    const bool current_can_accumulate =
        rmc.sog_knots >= sog_threshold && !estimate.below_threshold;

    if (m_ytd.year == 0) {
        m_ytd.year = rmc.year;
    }

    if (!m_has_last_fix) {
        m_last_fix = rmc;
        m_last_timestamp = timestamp;
        m_has_last_fix = true;
        m_last_fix_can_accumulate = current_can_accumulate;
        return false;
    }

    if (timestamp <= m_last_timestamp) {
        return false;
    }

    const double elapsed_seconds = static_cast<double>(timestamp - m_last_timestamp);
    const double distance_nm =
        haversine_nm(m_last_fix.latitude, m_last_fix.longitude, rmc.latitude, rmc.longitude);
    const double co2_tonnes = estimate.co2_rate_tonnes_hr * elapsed_seconds / 3600.0;
    const bool accumulated = current_can_accumulate && m_last_fix_can_accumulate;

    if (accumulated) {
        m_ytd.distance_nm += distance_nm;
        m_ytd.co2_tonnes += co2_tonnes;

        if (m_current_voyage.active) {
            add_to_voyage(m_current_voyage, distance_nm, co2_tonnes, elapsed_seconds);
        }
    }

    m_last_fix = rmc;
    m_last_timestamp = timestamp;
    m_last_fix_can_accumulate = current_can_accumulate;

    return accumulated;
}

void Accumulator::set_ytd_seed(const double co2_tonnes, const double distance_nm) {
    if (co2_tonnes < 0.0 || distance_nm < 0.0) {
        throw std::invalid_argument("YTD seed values cannot be negative");
    }

    m_ytd.co2_tonnes -= m_ytd.seed_co2_tonnes;
    m_ytd.distance_nm -= m_ytd.seed_distance_nm;
    m_ytd.seed_co2_tonnes = co2_tonnes;
    m_ytd.seed_distance_nm = distance_nm;
    m_ytd.co2_tonnes += co2_tonnes;
    m_ytd.distance_nm += distance_nm;
}

bool Accumulator::check_year_rollover(const int current_year, const VesselProfile&) {
    if (m_ytd.year == 0) {
        m_ytd.year = current_year;
        return false;
    }

    if (current_year <= m_ytd.year) {
        return false;
    }

    m_archived_years.push_back(m_ytd);
    m_ytd = YTDState{};
    m_ytd.year = current_year;
    m_has_last_fix = false;
    m_last_fix_can_accumulate = false;
    return true;
}

VoyageState Accumulator::current_voyage() const {
    return m_current_voyage;
}

YTDState Accumulator::year_to_date() const {
    return m_ytd;
}

const std::vector<VoyageRecord>& Accumulator::voyage_history() const {
    return m_history;
}

const std::vector<YTDState>& Accumulator::archived_years() const {
    return m_archived_years;
}

AccumulatorState Accumulator::state() const {
    return AccumulatorState{
        m_current_voyage,
        m_ytd,
        m_last_fix,
        m_last_timestamp,
        m_has_last_fix,
        m_last_fix_can_accumulate,
        m_history,
        m_archived_years
    };
}

void Accumulator::restore_state(const AccumulatorState& state) {
    m_current_voyage = state.current_voyage;
    m_ytd = state.ytd;
    m_last_fix = state.last_fix;
    m_last_timestamp = state.last_timestamp;
    m_has_last_fix = state.has_last_fix;
    m_last_fix_can_accumulate = state.last_fix_can_accumulate;
    m_history = state.history;
    m_archived_years = state.archived_years;
}

} // namespace eexi_cii
