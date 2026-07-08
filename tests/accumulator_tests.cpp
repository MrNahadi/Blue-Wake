#include "data/accumulator.h"
#include "plugin/plugin_core.h"

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

void expect_status(
    eexi_cii::ProcessStatus actual,
    eexi_cii::ProcessStatus expected,
    const std::string& message
) {
    if (actual != expected) {
        std::ostringstream out;
        out << message << " expected status " << static_cast<int>(expected)
            << " got " << static_cast<int>(actual);
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
    profile.displacement = 90000.0;
    profile.admiralty_coefficient = 680.0;
    profile.sfoc = 175.0;
    profile.fuel_type = eexi_cii::FuelType::Hfo;
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

void rmc_to_running_aer_pipeline_produces_expected_result() {
    const eexi_cii::VesselProfile profile = bulk_profile();
    eexi_cii::Accumulator accumulator;

    const auto first = eexi_cii::parse_rmc(
        "$GPRMC,000000,A,0000.000,N,00000.000,E,012.0,000.0,010126,,*1A"
    );
    const auto second = eexi_cii::parse_rmc(
        "$GPRMC,010000,A,0100.000,N,00000.000,E,012.0,000.0,010126,,*1A"
    );

    expect_true(first.valid, "First RMC parses");
    expect_true(second.valid, "Second RMC parses");

    accumulator.add_data_point(first, eexi_cii::estimate_fuel(first.sog_knots, profile));
    const bool accumulated =
        accumulator.add_data_point(second, eexi_cii::estimate_fuel(second.sog_knots, profile));

    expect_true(accumulated, "Second fix accumulates");
    expect_near(accumulator.year_to_date().distance_nm, 60.040460733, 0.000001, "Pipeline distance");
    expect_near(accumulator.year_to_date().co2_tonnes, 2.781120600, 0.000001, "Pipeline CO2");

    const auto result = eexi_cii::compute_aer(
        accumulator.year_to_date().co2_tonnes,
        accumulator.year_to_date().distance_nm,
        profile,
        accumulator.year_to_date().year
    );

    expect_near(result.aer, 0.579009672, 0.000001, "Pipeline running AER");
    expect_true(result.rating == 'A', "Pipeline CII rating");
}

void plugin_core_ignores_invalid_sentences_without_state_change() {
    eexi_cii::PluginCore core(bulk_profile());

    const auto result = core.process_nmea_sentence("$GPRMC,bad*00");

    expect_status(result.status, eexi_cii::ProcessStatus::InvalidSentence, "Invalid sentence status");
    expect_false(result.snapshot.has_rmc, "Invalid sentence does not create RMC snapshot");
    expect_false(result.snapshot.has_aer, "Invalid sentence does not create AER snapshot");
}

void plugin_core_turns_rmc_stream_into_dashboard_snapshot() {
    eexi_cii::PluginCore core(bulk_profile());

    const auto first = core.process_nmea_sentence(
        "$GPRMC,000000,A,0000.000,N,00000.000,E,012.0,000.0,010126,,*1A"
    );
    const auto second = core.process_nmea_sentence(
        "$GPRMC,010000,A,0100.000,N,00000.000,E,012.0,000.0,010126,,*1A"
    );

    expect_status(first.status, eexi_cii::ProcessStatus::BaselineOnly, "First plugin point");
    expect_true(first.snapshot.has_rmc, "First point has RMC snapshot");
    expect_false(first.snapshot.has_aer, "First point has no AER yet");

    expect_status(second.status, eexi_cii::ProcessStatus::Accumulated, "Second plugin point");
    expect_true(second.snapshot.has_aer, "Second point has AER snapshot");
    expect_near(second.snapshot.ytd.distance_nm, 60.040460733, 0.000001, "Plugin snapshot distance");
    expect_near(second.snapshot.ytd.co2_tonnes, 2.781120600, 0.000001, "Plugin snapshot CO2");
    expect_near(second.snapshot.aer_result.aer, 0.579009672, 0.000001, "Plugin snapshot AER");
    expect_true(second.snapshot.aer_result.rating == 'A', "Plugin snapshot rating");
}

void plugin_core_reports_below_threshold_exclusion() {
    eexi_cii::PluginCore core(bulk_profile());

    const auto result = core.process_nmea_sentence(
        "$GPRMC,000000,A,0000.000,N,00000.000,E,000.5,000.0,010126,,*1C"
    );

    expect_status(result.status, eexi_cii::ProcessStatus::Excluded, "Below-threshold status");
    expect_true(result.snapshot.has_rmc, "Below-threshold point has RMC snapshot");
    expect_false(result.snapshot.has_aer, "Below-threshold point has no AER snapshot");
    expect_near(result.snapshot.ytd.distance_nm, 0.0, 0.0, "Below-threshold distance");
}

void plugin_core_applies_profile_settings_to_runtime_state() {
    eexi_cii::ProfileSettings settings;
    settings.has_profile = true;
    settings.profile = bulk_profile();
    settings.profile.name = "MV Runtime";
    settings.profile.imo_number = "1234567";
    settings.profile.gt = 50000.0;
    settings.profile.dwt = 90000.0;
    settings.sog_threshold = 2.0;
    settings.target_year_override = 2026;
    settings.ytd_seed_co2_tonnes = 12.5;
    settings.ytd_seed_distance_nm = 100.0;

    eexi_cii::PluginCore core(bulk_profile());
    core.apply_settings(settings);

    expect_near(core.profile().dwt, 90000.0, 0.0, "Applied profile DWT");
    expect_near(core.sog_threshold(), 2.0, 0.0, "Applied SOG threshold");
    expect_equal(core.target_year_override(), 2026, "Applied target year");
    expect_near(core.snapshot().ytd.co2_tonnes, 12.5, 0.0, "Applied seed CO2");
    expect_near(core.snapshot().ytd.distance_nm, 100.0, 0.0, "Applied seed distance");

    const auto result = core.process_nmea_sentence(
        "$GPRMC,000000,A,0000.000,N,00000.000,E,001.5,000.0,010126,,*1D"
    );

    expect_status(result.status, eexi_cii::ProcessStatus::Excluded, "Applied threshold excludes");
}

void plugin_core_rejects_incomplete_profile_settings() {
    eexi_cii::PluginCore core(bulk_profile());

    bool threw = false;
    try {
        core.apply_settings(eexi_cii::ProfileSettings{});
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    expect_true(threw, "Incomplete settings are rejected");
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
        {"RMC to running AER pipeline produces expected result", rmc_to_running_aer_pipeline_produces_expected_result},
        {"PluginCore ignores invalid sentences without state change", plugin_core_ignores_invalid_sentences_without_state_change},
        {"PluginCore turns RMC stream into dashboard snapshot", plugin_core_turns_rmc_stream_into_dashboard_snapshot},
        {"PluginCore reports below-threshold exclusion", plugin_core_reports_below_threshold_exclusion},
        {"PluginCore applies Profile settings to runtime state", plugin_core_applies_profile_settings_to_runtime_state},
        {"PluginCore rejects incomplete Profile settings", plugin_core_rejects_incomplete_profile_settings},
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
