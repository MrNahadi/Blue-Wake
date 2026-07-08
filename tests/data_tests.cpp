#include "data/haversine.h"
#include "data/nmea_parser.h"

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

void expect_equal(const int actual, const int expected, const std::string& message) {
    if (actual != expected) {
        std::ostringstream out;
        out << message << " expected " << expected << " got " << actual;
        throw TestFailure(out.str());
    }
}

using Test = std::pair<std::string, std::function<void()>>;

void checksum_accepts_known_rmc_sentence() {
    expect_true(
        eexi_cii::validate_checksum(
            "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A"
        ),
        "Known RMC checksum"
    );
}

void valid_rmc_sentence_parses_navigation_data() {
    const auto rmc = eexi_cii::parse_rmc(
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A"
    );

    expect_true(rmc.valid, "RMC is valid");
    expect_near(rmc.utc_seconds, 45319.0, 0.000001, "UTC seconds");
    expect_near(rmc.latitude, 48.1173, 0.000001, "Latitude");
    expect_near(rmc.longitude, 11.5166667, 0.000001, "Longitude");
    expect_near(rmc.sog_knots, 22.4, 0.000001, "SOG");
    expect_near(rmc.cog_degrees, 84.4, 0.000001, "COG");
    expect_equal(rmc.day, 23, "Day");
    expect_equal(rmc.month, 3, "Month");
    expect_equal(rmc.year, 1994, "Year");
}

void rmc_accepts_multi_constellation_talker_id() {
    const auto rmc = eexi_cii::parse_rmc(
        "$GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*74"
    );

    expect_true(rmc.valid, "GN talker ID accepted");
    expect_near(rmc.sog_knots, 22.4, 0.000001, "GN SOG");
}

void bad_checksum_is_rejected() {
    const auto rmc = eexi_cii::parse_rmc(
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*00"
    );

    expect_false(rmc.valid, "Bad checksum rejected");
}

void void_status_is_rejected() {
    const auto rmc = eexi_cii::parse_rmc(
        "$GPRMC,123519,V,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*7D"
    );

    expect_false(rmc.valid, "Void status rejected");
}

void empty_sog_is_rejected_without_becoming_zero() {
    const auto rmc = eexi_cii::parse_rmc(
        "$GPRMC,123519,A,4807.038,N,01131.000,E,,084.4,230394,003.1,W*40"
    );

    expect_false(rmc.valid, "Empty SOG rejected");
    expect_near(rmc.sog_knots, -1.0, 0.0, "Empty SOG sentinel remains -1");
}

void haversine_returns_zero_for_same_position() {
    expect_near(
        eexi_cii::haversine_nm(1.0, 104.0, 1.0, 104.0),
        0.0,
        0.0,
        "Same position distance"
    );
}

void haversine_one_degree_latitude_is_about_sixty_nm() {
    expect_near(
        eexi_cii::haversine_nm(0.0, 0.0, 1.0, 0.0),
        60.04046,
        0.001,
        "One degree latitude distance"
    );
}

} // namespace

int main() {
    const std::vector<Test> tests = {
        {"Checksum accepts known RMC sentence", checksum_accepts_known_rmc_sentence},
        {"Valid RMC sentence parses navigation data", valid_rmc_sentence_parses_navigation_data},
        {"RMC accepts multi-constellation talker ID", rmc_accepts_multi_constellation_talker_id},
        {"Bad checksum is rejected", bad_checksum_is_rejected},
        {"Void status is rejected", void_status_is_rejected},
        {"Empty SOG is rejected without becoming zero", empty_sog_is_rejected_without_becoming_zero},
        {"Haversine returns zero for same position", haversine_returns_zero_for_same_position},
        {"Haversine one degree latitude is about sixty nm", haversine_one_degree_latitude_is_about_sixty_nm},
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
