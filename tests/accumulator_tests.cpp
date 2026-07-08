#include "data/accumulator.h"

#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string& message)
        : std::runtime_error(message) {}
};

void expect_true(bool value, const std::string& message) {
    if (!value) {
        throw TestFailure(message);
    }
}

void expect_false(bool value, const std::string& message) {
    expect_true(!value, message);
}

void expect_equal(int actual, int expected, const std::string& message) {
    if (actual != expected) {
        std::ostringstream out;
        out << message << " expected " << expected << " got " << actual;
        throw TestFailure(out.str());
    }
}

void expect_equal_size(std::size_t actual, std::size_t expected, const std::string& message) {
    if (actual != expected) {
        std::ostringstream out;
        out << message << " expected " << expected << " got " << actual;
        throw TestFailure(out.str());
    }
}

void expect_near(double actual, double expected, double tolerance, const std::string& message) {
    if (std::fabs(actual - expected) > tolerance) {
        std::ostringstream out;
        out << message << " expected " << expected << " got " << actual;
        throw TestFailure(out.str());
    }
}

using Test = std::pair<std::string, std::function<void()>>;

eexi_cii::RMCData fix(
    int hour,
    int minute,
    int second,
    double latitude,
    double longitude,
    double sog
) {
    eexi_cii::RMCData rmc;
    rmc.valid = true;
    rmc.utc_seconds = hour * 3600.0 + minute * 60.0 + second;
    rmc.latitude = latitude;
    rmc.longitude = longitude;
    rmc.sog_knots = sog;
    rmc.day = 1;
    rmc.month = 1;
    rmc.year = 2026;
    return rmc;
}

eexi_cii::FuelEstimate estimate(double co2_rate, bool below_threshold = false) {
    eexi_cii::FuelEstimate value;
    value.co2_rate_tonnes_hr = co2_rate;
    value.below_threshold = below_threshold;
    return value;
}

eexi_cii::VesselProfile bulk_profile() {
    eexi_cii::VesselProfile profile;
    profile.ship_type = eexi_cii::ShipType::BulkCarrier;
    profile.dwt = 80000.0;
    return profile;
}

void first_fix_sets_baseline_without_accumulating() {
    eexi_cii::Accumulator accumulator;

    const bool accumulated =
        accumulator.add_data_point(fix(0, 0, 0, 0.0, 0.0, 12.0), estimate(2.0));

    expect_false(accumulated, "First fix cannot produce a distance delta");
    expect_equal(accumulator.year_to_date().year, 2026, "YTD year is initialized");
    expect_near(accumulator.year_to_date().distance_nm, 0.0, 0.0, "Initial distance");
    expect_near(accumulator.year_to_date().co2_tonnes, 0.0, 0.0, "Initial CO2");
}

void second_underway_fix_accumulates_distance_and_elapsed_co2() {
    eexi_cii::Accumulator accumulator;

    accumulator.add_data_point(fix(0, 0, 0, 0.0, 0.0, 12.0), estimate(2.0));
    const bool accumulated =
        accumulator.add_data_point(fix(1, 0, 0, 1.0, 0.0, 12.0), estimate(2.0));

    expect_true(accumulated, "Second underway fix accumulates");
    expect_near(
        accumulator.year_to_date().distance_nm,
        60.040460733,
        0.000001,
        "One degree latitude distance"
    );
    expect_near(accumulator.year_to_date().co2_tonnes, 2.0, 0.000001, "One hour CO2");
}

void below_threshold_fix_breaks_accumulation_interval() {
    eexi_cii::Accumulator accumulator;

    accumulator.add_data_point(fix(0, 0, 0, 0.0, 0.0, 12.0), estimate(2.0));
    expect_false(
        accumulator.add_data_point(fix(1, 0, 0, 0.5, 0.0, 0.5), estimate(0.0, true)),
        "Below threshold point is excluded"
    );
    expect_false(
        accumulator.add_data_point(fix(2, 0, 0, 1.0, 0.0, 12.0), estimate(2.0)),
        "First point after excluded interval becomes new baseline"
    );

    expect_near(accumulator.year_to_date().distance_nm, 0.0, 0.0, "No threshold-gap distance");
    expect_near(accumulator.year_to_date().co2_tonnes, 0.0, 0.0, "No threshold-gap CO2");
}

void ytd_seed_is_included_once_and_can_be_replaced() {
    eexi_cii::Accumulator accumulator;

    accumulator.set_ytd_seed(100.0, 500.0);
    accumulator.set_ytd_seed(125.0, 550.0);

    expect_near(accumulator.year_to_date().co2_tonnes, 125.0, 0.0, "Seed CO2 replaced");
    expect_near(accumulator.year_to_date().distance_nm, 550.0, 0.0, "Seed distance replaced");
    expect_near(accumulator.year_to_date().seed_co2_tonnes, 125.0, 0.0, "Seed CO2 stored");
    expect_near(accumulator.year_to_date().seed_distance_nm, 550.0, 0.0, "Seed distance stored");
}

void active_voyage_receives_accumulated_totals_and_creates_record() {
    eexi_cii::Accumulator accumulator;
    accumulator.add_data_point(fix(0, 0, 0, 0.0, 0.0, 12.0), estimate(2.0));

    accumulator.start_voyage("Test Voyage", 90000.0);
    accumulator.add_data_point(fix(1, 0, 0, 1.0, 0.0, 12.0), estimate(2.0));

    expect_true(accumulator.has_active_voyage(), "Voyage is active");
    expect_near(accumulator.current_voyage().co2_tonnes, 2.0, 0.000001, "Voyage CO2");
    expect_near(
        accumulator.current_voyage().distance_nm,
        60.040460733,
        0.000001,
        "Voyage distance"
    );

    accumulator.end_voyage(bulk_profile(), 2026);

    expect_false(accumulator.has_active_voyage(), "Voyage ended");
    expect_equal_size(accumulator.voyage_history().size(), 1, "One voyage record");
    expect_near(accumulator.voyage_history()[0].aer, 0.416385, 0.000001, "Voyage AER");
}

void year_rollover_archives_and_resets_ytd_state() {
    eexi_cii::Accumulator accumulator;
    accumulator.add_data_point(fix(0, 0, 0, 0.0, 0.0, 12.0), estimate(2.0));
    accumulator.add_data_point(fix(1, 0, 0, 1.0, 0.0, 12.0), estimate(2.0));

    const bool rolled = accumulator.check_year_rollover(2027, bulk_profile());

    expect_true(rolled, "Year rollover occurred");
    expect_equal(accumulator.year_to_date().year, 2027, "New YTD year");
    expect_near(accumulator.year_to_date().co2_tonnes, 0.0, 0.0, "New YTD CO2 reset");
    expect_equal_size(accumulator.archived_years().size(), 1, "One archived year");
    expect_equal(accumulator.archived_years()[0].year, 2026, "Archived year");
}

} // namespace

int main() {
    const std::vector<Test> tests = {
        {"First fix sets baseline without accumulating", first_fix_sets_baseline_without_accumulating},
        {"Second underway fix accumulates distance and elapsed CO2", second_underway_fix_accumulates_distance_and_elapsed_co2},
        {"Below-threshold fix breaks accumulation interval", below_threshold_fix_breaks_accumulation_interval},
        {"YTD seed is included once and can be replaced", ytd_seed_is_included_once_and_can_be_replaced},
        {"Active Voyage receives accumulated totals and creates record", active_voyage_receives_accumulated_totals_and_creates_record},
        {"Year rollover archives and resets YTD state", year_rollover_archives_and_resets_ytd_state},
    };

    int failures = 0;

    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "[PASS] " << test.first << '\n';
        } catch (const std::exception& ex) {
            ++failures;
            std::cerr << "[FAIL] " << test.first << ": " << ex.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " test(s) passed\n";
    return 0;
}
