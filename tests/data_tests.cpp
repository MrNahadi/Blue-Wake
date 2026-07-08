#include "data/haversine.h"
#include "data/nmea_parser.h"
#include "export/csv_exporter.h"

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

void expect_equal(
    const std::string& actual,
    const std::string& expected,
    const std::string& message
) {
    if (actual != expected) {
        std::ostringstream out;
        out << message << " expected [" << expected << "] got [" << actual << "]";
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

void annual_csv_exports_ytd_summary_with_capacity_and_rating() {
    eexi_cii::VesselProfile profile;
    profile.name = "MV Example, One";
    profile.imo_number = "1234567";
    profile.ship_type = eexi_cii::ShipType::BulkCarrier;
    profile.dwt = 80000.0;

    eexi_cii::YTDState ytd;
    ytd.year = 2026;
    ytd.distance_nm = 48210.25;
    ytd.co2_tonnes = 12847.5;

    eexi_cii::AERResult result;
    result.aer = 3.330123;
    result.rating = 'B';
    result.required_cii = 3.766208;

    const std::string csv = eexi_cii::export_annual_csv(ytd, profile, result);

    expect_equal(
        csv,
        "Year,Ship Name,IMO Number,Ship Type,Capacity Type,Capacity,Total Distance (nm),"
        "Total CO2 (tonnes),AER or cgDIST,CII Rating,Required CII\n"
        "2026,\"MV Example, One\",1234567,Bulk Carrier,DWT,80000,48210.25,12847.5,3.330123,B,3.766208\n",
        "Annual CSV"
    );
}

void voyage_csv_exports_named_and_unassigned_records() {
    std::vector<eexi_cii::VoyageRecord> records;
    records.push_back(eexi_cii::VoyageRecord{
        "Rotterdam-Singapore",
        1768464000,
        1770916200,
        92000.0,
        4892.25,
        1247.5,
        3.19,
        false
    });
    records.push_back(eexi_cii::VoyageRecord{
        "",
        1770916200,
        1771155600,
        92000.0,
        3.2,
        0.8,
        0.0,
        true
    });

    const std::string csv = eexi_cii::export_voyage_csv(records);

    expect_equal(
        csv,
        "Voyage,Start,End,Displacement,Distance (nm),CO2 (tonnes),AER,Type\n"
        "Rotterdam-Singapore,2026-01-15T08:00:00Z,2026-02-12T17:10:00Z,92000,4892.25,1247.5,3.19,Named\n"
        "Unassigned,2026-02-12T17:10:00Z,2026-02-15T11:40:00Z,92000,3.2,0.8,0,Unassigned\n",
        "Voyage CSV"
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
        {"Annual CSV exports YTD summary with capacity and rating", annual_csv_exports_ytd_summary_with_capacity_and_rating},
        {"Voyage CSV exports named and unassigned records", voyage_csv_exports_named_and_unassigned_records},
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
