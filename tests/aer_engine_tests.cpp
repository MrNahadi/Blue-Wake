#include "domain/aer_engine.h"
#include "domain/cii_reference.h"

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

void expect_true(bool value, const std::string& message) {
    if (!value) {
        throw TestFailure(message);
    }
}

void expect_throws(const std::function<void()>& action, const std::string& message) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return;
    }

    throw TestFailure(message);
}

using Test = std::pair<std::string, std::function<void()>>;

eexi_cii::VesselProfile bulk_profile() {
    eexi_cii::VesselProfile profile;
    profile.ship_type = eexi_cii::ShipType::BulkCarrier;
    profile.dwt = 80000.0;
    return profile;
}

void confirmed_reduction_factors_are_available() {
    expect_near(eexi_cii::reduction_factor_z(2023), 5.0, 0.0, "2023 Z");
    expect_near(eexi_cii::reduction_factor_z(2024), 7.0, 0.0, "2024 Z");
    expect_near(eexi_cii::reduction_factor_z(2025), 9.0, 0.0, "2025 Z");
    expect_near(eexi_cii::reduction_factor_z(2026), 11.0, 0.0, "2026 Z");
}

void unsupported_future_reduction_factor_fails_loudly() {
    expect_throws([]() { eexi_cii::reduction_factor_z(2027); }, "2027 is not implemented");
}

void bulk_carrier_required_cii_and_boundaries_match_hand_calculation() {
    const double required =
        eexi_cii::required_cii(eexi_cii::ShipType::BulkCarrier, 80000.0, 2026);
    const auto boundaries =
        eexi_cii::rating_boundaries(eexi_cii::ShipType::BulkCarrier, 80000.0, 2026);

    expect_near(required, 3.766207566, 0.0000005, "Required CII");
    expect_near(boundaries.ab, 3.238938507, 0.0000005, "A/B boundary");
    expect_near(boundaries.bc, 3.540235112, 0.0000005, "B/C boundary");
    expect_near(boundaries.cd, 3.992180020, 0.0000005, "C/D boundary");
    expect_near(boundaries.de, 4.444124928, 0.0000005, "D/E boundary");
}

void aer_result_converts_co2_tonnes_to_grams() {
    const auto result = eexi_cii::compute_aer(5000.0, 10000.0, bulk_profile(), 2026);

    expect_near(result.aer, 6.25, 0.000001, "AER");
    expect_equal(result.rating, 'E', "Rating");
    expect_near(result.required_cii, 3.766207566, 0.0000005, "Required CII");
}

void rating_boundary_edges_use_lower_rating_until_exceeded() {
    const auto boundaries =
        eexi_cii::rating_boundaries(eexi_cii::ShipType::BulkCarrier, 80000.0, 2026);

    expect_equal(
        eexi_cii::cii_rating(boundaries.ab, eexi_cii::ShipType::BulkCarrier, 80000.0, 2026),
        'A',
        "A/B boundary is A"
    );
    expect_equal(
        eexi_cii::cii_rating(boundaries.bc, eexi_cii::ShipType::BulkCarrier, 80000.0, 2026),
        'B',
        "B/C boundary is B"
    );
    expect_equal(
        eexi_cii::cii_rating(boundaries.cd, eexi_cii::ShipType::BulkCarrier, 80000.0, 2026),
        'C',
        "C/D boundary is C"
    );
    expect_equal(
        eexi_cii::cii_rating(boundaries.de, eexi_cii::ShipType::BulkCarrier, 80000.0, 2026),
        'D',
        "D/E boundary is D"
    );
}

void cgdist_result_uses_gt_capacity() {
    eexi_cii::VesselProfile profile;
    profile.ship_type = eexi_cii::ShipType::CruisePassenger;
    profile.gt = 90000.0;

    expect_true(profile.cii_capacity() == 90000.0, "Cruise profile uses GT capacity");
    expect_throws(
        [&]() { eexi_cii::compute_aer(1000.0, 1000.0, profile, 2026); },
        "Cruise passenger coefficients not yet implemented"
    );
}

} // namespace

int main() {
    const std::vector<Test> tests = {
        {"Confirmed reduction factors are available", confirmed_reduction_factors_are_available},
        {"Unsupported future reduction factor fails loudly", unsupported_future_reduction_factor_fails_loudly},
        {"Bulk carrier required CII and boundaries match hand calculation", bulk_carrier_required_cii_and_boundaries_match_hand_calculation},
        {"AER result converts CO2 tonnes to grams", aer_result_converts_co2_tonnes_to_grams},
        {"Rating boundary edges use lower rating until exceeded", rating_boundary_edges_use_lower_rating_until_exceeded},
        {"cgDIST result uses GT capacity before unsupported coefficients fail", cgdist_result_uses_gt_capacity},
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
