#ifndef EEXI_CII_DATA_ACCUMULATOR_H
#define EEXI_CII_DATA_ACCUMULATOR_H

#include "data/nmea_parser.h"
#include "domain/aer_engine.h"
#include "domain/fuel_estimator.h"
#include "domain/vessel_profile.h"

#include <cstdint>
#include <string>
#include <vector>

namespace eexi_cii {

struct VoyageState {
    std::string name;
    double displacement = 0.0;
    double distance_nm = 0.0;
    double co2_tonnes = 0.0;
    double elapsed_seconds = 0.0;
    std::int64_t start_timestamp = 0;
    bool active = false;
};

struct VoyageRecord {
    std::string name;
    std::int64_t start_timestamp = 0;
    std::int64_t end_timestamp = 0;
    double displacement = 0.0;
    double distance_nm = 0.0;
    double co2_tonnes = 0.0;
    double aer = 0.0;
    bool is_unassigned = false;
};

struct YTDState {
    int year = 0;
    double distance_nm = 0.0;
    double co2_tonnes = 0.0;
    double seed_distance_nm = 0.0;
    double seed_co2_tonnes = 0.0;
};

struct AccumulatorState {
    VoyageState current_voyage;
    YTDState ytd;
    RMCData last_fix;
    std::int64_t last_timestamp = 0;
    bool has_last_fix = false;
    bool last_fix_can_accumulate = false;
    std::vector<VoyageRecord> history;
    std::vector<YTDState> archived_years;
};

class Accumulator {
public:
    void start_voyage(const std::string& name, double displacement);
    void end_voyage(const VesselProfile& profile, int year);
    bool has_active_voyage() const;

    bool add_data_point(
        const RMCData& rmc,
        const FuelEstimate& estimate,
        double sog_threshold = 1.0
    );

    void set_ytd_seed(double co2_tonnes, double distance_nm);
    bool check_year_rollover(int current_year, const VesselProfile& profile);

    VoyageState current_voyage() const;
    YTDState year_to_date() const;
    const std::vector<VoyageRecord>& voyage_history() const;
    const std::vector<YTDState>& archived_years() const;
    AccumulatorState state() const;
    void restore_state(const AccumulatorState& state);

private:
    VoyageState m_current_voyage;
    YTDState m_ytd;
    RMCData m_last_fix;
    std::int64_t m_last_timestamp = 0;
    bool m_has_last_fix = false;
    bool m_last_fix_can_accumulate = false;
    std::vector<VoyageRecord> m_history;
    std::vector<YTDState> m_archived_years;
};

std::int64_t rmc_timestamp_seconds(const RMCData& rmc);

} // namespace eexi_cii

#endif
