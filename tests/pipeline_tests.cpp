#include "data/accumulator.h"
#include "data/nmea_parser.h"
#include "domain/aer_engine.h"
#include "domain/fuel_estimator.h"

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

void expect_near(double actual, double expected, double tolerance, const std::string& message) {
    if (std::fabs(actual - expected) > tolerance) {
        std::ostringstream out;
        out << message << " expected " << expected << " got " << actual;
        throw TestFailure(out.str());
    }
}

void expect_equal(char actual, char expected, const std::string& message) {
    if (actual != expected) {
        std::ostringstream out;
        out << message << " expected " << expected << " got " << actual;
        throw TestFailure(out.str());
    }
}

using Test = std::pair<std::string, std::function<void()>>;

eexi_cii::VesselProfile validation_profile() {
    eexi_cii::VesselProfile profile;
    profile.ship_type = eexi_cii::ShipType::BulkCarrier;
    profile.dwt = 80000.0;
    profile.displacement = 90000.0;
    profile.admiralty_coefficient = 680.0;
    profile.sfoc = 175.0;
    profile.fuel_type = eexi_cii::FuelType::Hfo;
    return profile;
}

void rmc_to_running_aer_pipeline_produces_expected_result() {
    const eexi_cii::VesselProfile profile = validation_profile();
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
    expect_near(
        accumulator.year_to_date().distance_nm,
        60.040460733,
        0.000001,
        "Pipeline distance"
    );
    expect_near(
        accumulator.year_to_date().co2_tonnes,
        2.781120600,
        0.000001,
        "Pipeline CO2"
    );

    const auto result = eexi_cii::compute_aer(
        accumulator.year_to_date().co2_tonnes,
        accumulator.year_to_date().distance_nm,
        profile,
        accumulator.year_to_date().year
    );

    expect_near(result.aer, 0.579009672, 0.000001, "Pipeline running AER");
    expect_equal(result.rating, 'A', "Pipeline CII rating");
}

} // namespace

int main() {
    const std::vector<Test> tests = {
        {"RMC to running AER pipeline produces expected result", rmc_to_running_aer_pipeline_produces_expected_result},
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
