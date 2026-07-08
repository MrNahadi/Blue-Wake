#include "domain/fuel_estimator.h"
#include "domain/vessel_profile.h"

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

void expect_true(const bool value, const std::string& message) {
    if (!value) {
        throw TestFailure(message);
    }
}

void expect_false(const bool value, const std::string& message) {
    expect_true(!value, message);
}

void expect_near(
    const double actual,
    const double expected,
    const double tolerance,
    const std::string& message
) {
    if (std::fabs(actual - expected) > tolerance) {
        std::ostringstream out;
        out << message << " expected " << expected << " got " << actual
            << " tolerance " << tolerance;
        throw TestFailure(out.str());
    }
}

void expect_equal(
    const double actual,
    const double expected,
    const std::string& message
) {
    expect_near(actual, expected, 0.0, message);
}

void expect_throws(
    const std::function<void()>& action,
    const std::string& message
) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return;
    }

    throw TestFailure(message);
}

using Test = std::pair<std::string, std::function<void()>>;

void emission_factors_match_imo_values() {
    using eexi_cii::FuelType;
    using eexi_cii::emission_factor;

    expect_near(emission_factor(FuelType::Hfo), 3.114, 0.000001, "HFO C_F");
    expect_near(emission_factor(FuelType::Lfo), 3.151, 0.000001, "LFO C_F");
    expect_near(emission_factor(FuelType::Mdo), 3.206, 0.000001, "MDO C_F");
    expect_near(emission_factor(FuelType::Lng), 2.750, 0.000001, "LNG C_F");
    expect_near(emission_factor(FuelType::Methanol), 1.375, 0.000001, "Methanol C_F");
}

void capacity_uses_dwt_for_aer_types() {
    eexi_cii::VesselProfile profile;
    profile.ship_type = eexi_cii::ShipType::BulkCarrier;
    profile.dwt = 80000.0;
    profile.gt = 43000.0;

    expect_false(eexi_cii::uses_cgdist(profile.ship_type), "Bulk carrier uses AER");
    expect_equal(profile.cii_capacity(), 80000.0, "AER capacity uses DWT");
}

void capacity_uses_gt_for_cgdist_types() {
    eexi_cii::VesselProfile profile;
    profile.ship_type = eexi_cii::ShipType::CruisePassenger;
    profile.dwt = 12000.0;
    profile.gt = 90000.0;

    expect_true(eexi_cii::uses_cgdist(profile.ship_type), "Cruise passenger uses cgDIST");
    expect_equal(profile.cii_capacity(), 90000.0, "cgDIST capacity uses GT");
}

void bulk_carrier_fuel_estimate_matches_hand_calculation() {
    eexi_cii::VesselProfile profile;
    profile.displacement = 90000.0;
    profile.admiralty_coefficient = 680.0;
    profile.sfoc = 175.0;
    profile.fuel_type = eexi_cii::FuelType::Hfo;

    const auto estimate = eexi_cii::estimate_fuel(12.5, profile);

    expect_near(estimate.shaft_power_kw, 5768.321606, 0.0005, "Shaft power");
    expect_near(estimate.fuel_rate_tonnes_hr, 1.009456281, 0.0000005, "Fuel rate");
    expect_near(estimate.co2_rate_tonnes_hr, 3.143446859, 0.0000005, "CO2 rate");
    expect_false(estimate.below_threshold, "Service speed is above threshold");
}

void below_threshold_returns_zero_estimate() {
    eexi_cii::VesselProfile profile;
    profile.displacement = 90000.0;
    profile.admiralty_coefficient = 680.0;
    profile.sfoc = 175.0;

    const auto estimate = eexi_cii::estimate_fuel(0.5, profile);

    expect_equal(estimate.shaft_power_kw, 0.0, "Sub-threshold shaft power");
    expect_equal(estimate.fuel_rate_tonnes_hr, 0.0, "Sub-threshold fuel rate");
    expect_equal(estimate.co2_rate_tonnes_hr, 0.0, "Sub-threshold CO2 rate");
    expect_true(estimate.below_threshold, "Sub-threshold flag");
}

void invalid_profile_values_are_rejected() {
    eexi_cii::VesselProfile profile;
    profile.displacement = 90000.0;
    profile.admiralty_coefficient = 680.0;
    profile.sfoc = 175.0;

    profile.displacement = 0.0;
    expect_throws([&]() { eexi_cii::estimate_fuel(12.0, profile); }, "Zero displacement rejected");

    profile.displacement = 90000.0;
    profile.admiralty_coefficient = 0.0;
    expect_throws([&]() { eexi_cii::estimate_fuel(12.0, profile); }, "Zero Admiralty Coefficient rejected");

    profile.admiralty_coefficient = 680.0;
    profile.sfoc = -1.0;
    expect_throws([&]() { eexi_cii::estimate_fuel(12.0, profile); }, "Negative SFOC rejected");
}

} // namespace

int main() {
    const std::vector<Test> tests = {
        {"Emission factors match IMO values", emission_factors_match_imo_values},
        {"Capacity uses DWT for AER types", capacity_uses_dwt_for_aer_types},
        {"Capacity uses GT for cgDIST types", capacity_uses_gt_for_cgdist_types},
        {"Bulk carrier fuel estimate matches hand calculation", bulk_carrier_fuel_estimate_matches_hand_calculation},
        {"Below-threshold SOG returns zero estimate", below_threshold_returns_zero_estimate},
        {"Invalid profile values are rejected", invalid_profile_values_are_rejected},
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
