#include "plugin/plugin_core.h"

#include <cmath>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef EEXI_CII_TEST_FIXTURE_DIR
#define EEXI_CII_TEST_FIXTURE_DIR "tests/fixtures"
#endif

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

using Test = std::pair<std::string, std::function<void()>>;

eexi_cii::VesselProfile validation_profile() {
    eexi_cii::VesselProfile profile;
    profile.name = "MV Validation";
    profile.imo_number = "1234567";
    profile.ship_type = eexi_cii::ShipType::BulkCarrier;
    profile.dwt = 80000.0;
    profile.gt = 50000.0;
    profile.displacement = 90000.0;
    profile.admiralty_coefficient = 680.0;
    profile.sfoc = 175.0;
    profile.fuel_type = eexi_cii::FuelType::Hfo;
    return profile;
}

std::vector<std::string> replay_lines() {
    const std::string path =
        std::string(EEXI_CII_TEST_FIXTURE_DIR) + "/replay_validation_route.nmea";
    std::ifstream in(path);
    if (!in) {
        throw TestFailure("Could not open replay fixture: " + path);
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        lines.push_back(line);
    }
    return lines;
}

void synthetic_replay_matches_expected_distance_co2_and_rating() {
    eexi_cii::PluginCore core(validation_profile());
    eexi_cii::ProcessResult result;

    for (const std::string& line : replay_lines()) {
        result = core.process_nmea_sentence(line);
    }

    expect_true(result.status == eexi_cii::ProcessStatus::Accumulated, "Final replay point accumulated");
    expect_true(result.snapshot.has_aer, "Replay produced AER snapshot");
    expect_true(result.snapshot.profile.name == "MV Validation", "Snapshot carries vessel profile");
    expect_near(result.snapshot.ytd.distance_nm, 180.121382199, 0.000001, "Replay distance");
    expect_near(result.snapshot.ytd.co2_tonnes, 8.343361800, 0.000001, "Replay CO2");
    expect_near(result.snapshot.aer_result.aer, 0.579009672, 0.000001, "Replay AER");
    expect_true(result.snapshot.aer_result.rating == 'A', "Replay CII rating");
}

} // namespace

int main() {
    const std::vector<Test> tests = {
        {"Synthetic replay matches expected distance, CO2, and rating",
            synthetic_replay_matches_expected_distance_co2_and_rating},
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
