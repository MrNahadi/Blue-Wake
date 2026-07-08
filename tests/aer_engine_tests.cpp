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

void expect_equal(double actual, double expected, const std::string& message) {
    expect_near(actual, expected, 0.0, message);
}

void expect_true(bool value, const std::string& message) {
    if (!value) {
        throw TestFailure(message);
    }
}

void expect_coefficients(
    eexi_cii::ShipType ship_type,
    double capacity,
    double expected_a,
    double expected_c,
    double expected_d1,
    double expected_d2,
    double expected_d3,
    double expected_d4,
    const std::string& message
) {
    const auto coeffs = eexi_cii::cii_coefficients(ship_type, capacity);

    expect_near(coeffs.a, expected_a, 0.000001, message + " a");
    expect_near(coeffs.c, expected_c, 0.000001, message + " c");
    expect_near(coeffs.d1, expected_d1, 0.000001, message + " d1");
    expect_near(coeffs.d2, expected_d2, 0.000001, message + " d2");
    expect_near(coeffs.d3, expected_d3, 0.000001, message + " d3");
    expect_near(coeffs.d4, expected_d4, 0.000001, message + " d4");
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

void reference_coefficients_cover_modeled_ship_types() {
    using eexi_cii::ShipType;

    expect_coefficients(ShipType::BulkCarrier, 80000.0, 4745.0, 0.622, 0.86, 0.94, 1.06, 1.18, "Bulk carrier");
    expect_coefficients(ShipType::GasCarrierGte65k, 80000.0, 14405e7, 2.071, 0.81, 0.91, 1.12, 1.44, "Gas carrier >=65k");
    expect_coefficients(ShipType::GasCarrierLt65k, 50000.0, 8104.0, 0.639, 0.85, 0.95, 1.06, 1.25, "Gas carrier <65k");
    expect_coefficients(ShipType::Tanker, 50000.0, 5247.0, 0.610, 0.82, 0.93, 1.08, 1.28, "Tanker");
    expect_coefficients(ShipType::ContainerShip, 100000.0, 1984.0, 0.489, 0.83, 0.94, 1.07, 1.19, "Container");
    expect_coefficients(ShipType::GeneralCargo, 25000.0, 31948.0, 0.792, 0.83, 0.94, 1.06, 1.19, "General cargo >=20k");
    expect_coefficients(ShipType::GeneralCargo, 15000.0, 588.0, 0.3885, 0.83, 0.94, 1.06, 1.19, "General cargo <20k");
    expect_coefficients(ShipType::RefrigeratedCargo, 10000.0, 4600.0, 0.557, 0.78, 0.91, 1.07, 1.20, "Refrigerated cargo");
    expect_coefficients(ShipType::CombinationCarrier, 50000.0, 5119.0, 0.622, 0.87, 0.96, 1.06, 1.14, "Combination carrier");
    expect_coefficients(ShipType::LngCarrierGte100k, 150000.0, 9.827, 0.0, 0.89, 0.98, 1.06, 1.13, "LNG >=100k");
    expect_coefficients(ShipType::LngCarrierLt100k, 80000.0, 14479e10, 2.673, 0.78, 0.92, 1.10, 1.37, "LNG 65-100k");
    expect_coefficients(ShipType::LngCarrierLt100k, 50000.0, 14779e10, 2.673, 0.78, 0.92, 1.10, 1.37, "LNG <65k");
    expect_coefficients(ShipType::RoroVehicleCarrier, 60000.0, 3627.0, 0.590, 0.86, 0.94, 1.06, 1.16, "Vehicle carrier >=57.7k");
    expect_coefficients(ShipType::RoroVehicleCarrier, 40000.0, 3627.0, 0.590, 0.86, 0.94, 1.06, 1.16, "Vehicle carrier 30-57.7k");
    expect_coefficients(ShipType::RoroVehicleCarrier, 20000.0, 330.0, 0.329, 0.86, 0.94, 1.06, 1.16, "Vehicle carrier <30k");
    expect_coefficients(ShipType::RoroCargo, 30000.0, 1967.0, 0.485, 0.76, 0.89, 1.08, 1.27, "Ro-ro cargo");
    expect_coefficients(ShipType::RoroPassenger, 30000.0, 2023.0, 0.460, 0.76, 0.92, 1.14, 1.30, "Ro-ro passenger");
    expect_coefficients(ShipType::CruisePassenger, 90000.0, 930.0, 0.383, 0.87, 0.95, 1.06, 1.16, "Cruise passenger");
}

void reference_capacity_applies_imo_caps_and_floors() {
    using eexi_cii::ShipType;

    expect_equal(eexi_cii::cii_reference_capacity(ShipType::BulkCarrier, 300000.0), 279000.0, "Bulk carrier cap");
    expect_equal(eexi_cii::cii_reference_capacity(ShipType::BulkCarrier, 80000.0), 80000.0, "Bulk carrier normal capacity");
    expect_equal(eexi_cii::cii_reference_capacity(ShipType::LngCarrierLt100k, 50000.0), 65000.0, "LNG <65k floor");
    expect_equal(eexi_cii::cii_reference_capacity(ShipType::LngCarrierLt100k, 80000.0), 80000.0, "LNG 65-100k capacity");
    expect_equal(eexi_cii::cii_reference_capacity(ShipType::RoroVehicleCarrier, 60000.0), 57700.0, "Vehicle carrier cap");
    expect_equal(eexi_cii::cii_reference_capacity(ShipType::RoroVehicleCarrier, 40000.0), 40000.0, "Vehicle carrier mid capacity");
}

void representative_required_cii_values_match_hand_calculation() {
    using eexi_cii::ShipType;

    expect_near(eexi_cii::required_cii(ShipType::GasCarrierGte65k, 80000.0, 2026), 8.986775780, 0.0000005, "Gas carrier >=65k required CII");
    expect_near(eexi_cii::required_cii(ShipType::GeneralCargo, 15000.0, 2026), 12.484060900, 0.0000005, "General cargo <20k required CII");
    expect_near(eexi_cii::required_cii(ShipType::LngCarrierLt100k, 50000.0, 2026), 17.952198988, 0.0000005, "LNG <65k required CII");
    expect_near(eexi_cii::required_cii(ShipType::RoroVehicleCarrier, 60000.0, 2026), 5.010070446, 0.0000005, "Vehicle carrier >=57.7k required CII");
    expect_near(eexi_cii::required_cii(ShipType::CruisePassenger, 90000.0, 2026), 10.480887855, 0.0000005, "Cruise passenger required CII");
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
    const auto result = eexi_cii::compute_aer(1000.0, 10000.0, profile, 2026);

    expect_near(result.aer, 1.111111111, 0.0000005, "cgDIST attained value");
    expect_equal(result.rating, 'A', "Cruise passenger cgDIST rating");
    expect_true(result.uses_cgdist, "Cruise result is labelled cgDIST");
}

} // namespace

int main() {
    const std::vector<Test> tests = {
        {"Confirmed reduction factors are available", confirmed_reduction_factors_are_available},
        {"Unsupported future reduction factor fails loudly", unsupported_future_reduction_factor_fails_loudly},
        {"Bulk carrier required CII and boundaries match hand calculation", bulk_carrier_required_cii_and_boundaries_match_hand_calculation},
        {"Reference coefficients cover modeled ship types", reference_coefficients_cover_modeled_ship_types},
        {"Reference capacity applies IMO caps and floors", reference_capacity_applies_imo_caps_and_floors},
        {"Representative required CII values match hand calculation", representative_required_cii_values_match_hand_calculation},
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
