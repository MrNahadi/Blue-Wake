#include "data/haversine.h"
#include "data/nmea_parser.h"
#include "config/profile_settings.h"
#include "export/csv_exporter.h"
#include "data/persistence.h"

#include <cmath>
#include <cstdio>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <map>
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

void expect_equal_size(
    const std::size_t actual,
    const std::size_t expected,
    const std::string& message
) {
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

void expect_equal(const bool actual, const bool expected, const std::string& message) {
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

void profile_settings_round_trip_complete_vessel_profile() {
    eexi_cii::ProfileSettings settings;
    settings.has_profile = true;
    settings.profile.name = "MV Profile";
    settings.profile.imo_number = "9876543";
    settings.profile.gt = 42000.0;
    settings.profile.ship_type = eexi_cii::ShipType::ContainerShip;
    settings.profile.dwt = 71000.0;
    settings.profile.admiralty_coefficient = 645.5;
    settings.profile.sfoc = 178.0;
    settings.profile.fuel_type = eexi_cii::FuelType::Mdo;
    settings.profile.displacement = 85000.0;
    settings.profile.installed_power_mco = 11200.0;
    settings.profile.reference_speed = 16.4;
    settings.sog_threshold = 1.5;
    settings.target_year_override = 2026;
    settings.ytd_seed_co2_tonnes = 120.25;
    settings.ytd_seed_distance_nm = 950.5;

    const auto encoded = eexi_cii::encode_profile_settings(settings);

    expect_equal(encoded.at("Vessel/ShipType"), "container_ship", "Encoded ship type");
    expect_equal(encoded.at("Vessel/FuelType"), "mdo", "Encoded fuel type");
    expect_equal(encoded.at("Vessel/ProfileConfigured"), "true", "Encoded profile flag");

    const auto decoded = eexi_cii::decode_profile_settings(encoded);

    expect_false(decoded.setup_required(), "Complete profile does not require setup");
    expect_equal(decoded.has_profile, true, "Decoded profile flag");
    expect_equal(decoded.profile.name, "MV Profile", "Decoded vessel name");
    expect_equal(decoded.profile.imo_number, "9876543", "Decoded IMO");
    expect_near(decoded.profile.gt, 42000.0, 0.0, "Decoded GT");
    expect_true(
        decoded.profile.ship_type == eexi_cii::ShipType::ContainerShip,
        "Decoded ship type"
    );
    expect_near(decoded.profile.dwt, 71000.0, 0.0, "Decoded DWT");
    expect_near(decoded.profile.admiralty_coefficient, 645.5, 0.0, "Decoded admiralty coefficient");
    expect_near(decoded.profile.sfoc, 178.0, 0.0, "Decoded SFOC");
    expect_true(decoded.profile.fuel_type == eexi_cii::FuelType::Mdo, "Decoded fuel type");
    expect_near(decoded.profile.displacement, 85000.0, 0.0, "Decoded displacement");
    expect_near(decoded.profile.installed_power_mco, 11200.0, 0.0, "Decoded MCO");
    expect_near(decoded.profile.reference_speed, 16.4, 0.0, "Decoded reference speed");
    expect_near(decoded.sog_threshold, 1.5, 0.0, "Decoded SOG threshold");
    expect_equal(decoded.target_year_override, 2026, "Decoded target year");
    expect_near(decoded.ytd_seed_co2_tonnes, 120.25, 0.0, "Decoded seed CO2");
    expect_near(decoded.ytd_seed_distance_nm, 950.5, 0.0, "Decoded seed distance");
}

void empty_profile_settings_require_setup_and_keep_defaults() {
    const auto settings = eexi_cii::decode_profile_settings({});

    expect_true(settings.setup_required(), "Empty settings require setup");
    expect_equal(settings.has_profile, false, "Empty settings profile flag");
    expect_near(settings.sog_threshold, 1.0, 0.0, "Default SOG threshold");
    expect_equal(settings.target_year_override, 0, "Default target year auto");
    expect_near(settings.profile.sfoc, 190.0, 0.0, "Default SFOC");
}

void invalid_profile_settings_ship_type_is_rejected() {
    eexi_cii::SettingsMap values;
    values["Vessel/ProfileConfigured"] = "true";
    values["Vessel/ShipType"] = "spaceship";

    bool threw = false;
    try {
        (void)eexi_cii::decode_profile_settings(values);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    expect_true(threw, "Unknown ship type is rejected");
}

eexi_cii::RMCData persistence_fix(
    const int hour,
    const double latitude,
    const double longitude,
    const double sog,
    const int year = 2026
) {
    eexi_cii::RMCData rmc;
    rmc.valid = true;
    rmc.utc_seconds = hour * 3600.0;
    rmc.latitude = latitude;
    rmc.longitude = longitude;
    rmc.sog_knots = sog;
    rmc.day = 1;
    rmc.month = 1;
    rmc.year = year;
    return rmc;
}

eexi_cii::FuelEstimate persistence_estimate(const double co2_rate) {
    eexi_cii::FuelEstimate estimate;
    estimate.co2_rate_tonnes_hr = co2_rate;
    return estimate;
}

eexi_cii::VesselProfile persistence_profile() {
    eexi_cii::VesselProfile profile;
    profile.name = "MV Persistence";
    profile.imo_number = "7654321";
    profile.ship_type = eexi_cii::ShipType::BulkCarrier;
    profile.gt = 50000.0;
    profile.dwt = 80000.0;
    profile.displacement = 90000.0;
    profile.admiralty_coefficient = 680.0;
    profile.sfoc = 175.0;
    profile.fuel_type = eexi_cii::FuelType::Hfo;
    return profile;
}

void accumulator_json_round_trip_preserves_state_and_continues_from_last_fix() {
    eexi_cii::Accumulator accumulator;
    accumulator.set_ytd_seed(10.0, 100.0);
    accumulator.add_data_point(persistence_fix(0, 0.0, 0.0, 12.0), persistence_estimate(2.0));
    accumulator.start_voyage("Saved Voyage", 90000.0);
    accumulator.add_data_point(persistence_fix(1, 1.0, 0.0, 12.0), persistence_estimate(2.0));
    accumulator.end_voyage(persistence_profile(), 2026);
    accumulator.check_year_rollover(2027, persistence_profile());
    accumulator.start_voyage("Current Voyage", 91000.0);
    accumulator.add_data_point(persistence_fix(2, 2.0, 0.0, 12.0, 2027), persistence_estimate(2.0));

    const std::string json = eexi_cii::serialize_accumulator(accumulator);
    const eexi_cii::Accumulator restored = eexi_cii::deserialize_accumulator(json);

    expect_equal(restored.year_to_date().year, 2027, "Restored YTD year");
    expect_equal_size(restored.archived_years().size(), 1, "Restored archived years");
    expect_equal(restored.archived_years()[0].year, 2026, "Restored archived year value");
    expect_equal_size(restored.voyage_history().size(), 1, "Restored voyage history");
    expect_equal(restored.voyage_history()[0].name, "Saved Voyage", "Restored voyage name");
    expect_true(restored.has_active_voyage(), "Restored active voyage");
    expect_equal(restored.current_voyage().name, "Current Voyage", "Restored active voyage name");

    eexi_cii::Accumulator continued = restored;
    const bool accumulated = continued.add_data_point(
        persistence_fix(3, 3.0, 0.0, 12.0, 2027),
        persistence_estimate(2.0)
    );

    expect_true(accumulated, "Restored accumulator continues from saved last fix");
    expect_near(continued.year_to_date().distance_nm, 60.040460733, 0.000001, "Continued distance");
    expect_near(continued.year_to_date().co2_tonnes, 2.0, 0.000001, "Continued CO2");
    expect_near(continued.current_voyage().distance_nm, 60.040460733, 0.000001, "Continued voyage distance");
}

void accumulator_file_round_trip_preserves_ytd_seed() {
    const std::string filepath = "eexi_cii_accumulator_test.json";

    eexi_cii::Accumulator accumulator;
    accumulator.set_ytd_seed(42.0, 240.0);

    eexi_cii::save_accumulator(filepath, accumulator);
    const eexi_cii::Accumulator restored = eexi_cii::load_accumulator(filepath);
    std::remove(filepath.c_str());

    expect_near(restored.year_to_date().co2_tonnes, 42.0, 0.0, "File restored CO2 seed");
    expect_near(restored.year_to_date().distance_nm, 240.0, 0.0, "File restored distance seed");
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
        {"Profile settings round-trip complete Vessel Profile", profile_settings_round_trip_complete_vessel_profile},
        {"Empty Profile settings require setup and keep defaults", empty_profile_settings_require_setup_and_keep_defaults},
        {"Invalid Profile settings ship type is rejected", invalid_profile_settings_ship_type_is_rejected},
        {"Accumulator JSON round-trip preserves state and continues from last fix", accumulator_json_round_trip_preserves_state_and_continues_from_last_fix},
        {"Accumulator file round-trip preserves YTD seed", accumulator_file_round_trip_preserves_ytd_seed},
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
