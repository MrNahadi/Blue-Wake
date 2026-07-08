#include "config/profile_settings.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace eexi_cii {

namespace {

constexpr const char* kProfileConfigured = "Vessel/ProfileConfigured";
constexpr const char* kName = "Vessel/Name";
constexpr const char* kImoNumber = "Vessel/IMONumber";
constexpr const char* kGt = "Vessel/GT";
constexpr const char* kShipType = "Vessel/ShipType";
constexpr const char* kDwt = "Vessel/DWT";
constexpr const char* kAdmiraltyCoefficient = "Vessel/AdmiraltyCoefficient";
constexpr const char* kSfoc = "Vessel/SFOC";
constexpr const char* kFuelType = "Vessel/FuelType";
constexpr const char* kDisplacement = "Vessel/Displacement";
constexpr const char* kInstalledPowerMco = "EEXI/InstalledPowerMCO";
constexpr const char* kReferenceSpeed = "EEXI/ReferenceSpeed";
constexpr const char* kSogThreshold = "Advanced/SOGThreshold";
constexpr const char* kTargetYearOverride = "Advanced/TargetYearOverride";
constexpr const char* kYtdSeedCo2Tonnes = "Advanced/YTDSeedCO2Tonnes";
constexpr const char* kYtdSeedDistanceNm = "Advanced/YTDSeedDistanceNM";

std::string format_double(const double value) {
    std::ostringstream out;
    out << std::setprecision(15) << value;
    return out.str();
}

std::string get_or_default(
    const SettingsMap& values,
    const std::string& key,
    const std::string& default_value
) {
    const auto it = values.find(key);
    if (it == values.end()) {
        return default_value;
    }
    return it->second;
}

bool get_bool_or_default(
    const SettingsMap& values,
    const std::string& key,
    const bool default_value
) {
    const auto raw = get_or_default(values, key, default_value ? "true" : "false");
    if (raw == "true" || raw == "1") {
        return true;
    }
    if (raw == "false" || raw == "0") {
        return false;
    }
    throw std::invalid_argument("Invalid boolean setting for " + key);
}

double get_double_or_default(
    const SettingsMap& values,
    const std::string& key,
    const double default_value
) {
    const auto raw = get_or_default(values, key, format_double(default_value));
    std::size_t parsed = 0;
    const double value = std::stod(raw, &parsed);
    if (parsed != raw.size()) {
        throw std::invalid_argument("Invalid numeric setting for " + key);
    }
    return value;
}

int get_int_or_default(
    const SettingsMap& values,
    const std::string& key,
    const int default_value
) {
    const auto raw = get_or_default(values, key, std::to_string(default_value));
    std::size_t parsed = 0;
    const int value = std::stoi(raw, &parsed);
    if (parsed != raw.size()) {
        throw std::invalid_argument("Invalid integer setting for " + key);
    }
    return value;
}

} // namespace

bool ProfileSettings::setup_required() const {
    if (!has_profile) {
        return true;
    }
    if (profile.name.empty() || profile.imo_number.empty()) {
        return true;
    }
    if (profile.gt <= 0.0 || profile.cii_capacity() <= 0.0) {
        return true;
    }
    if (profile.admiralty_coefficient <= 0.0 || profile.sfoc <= 0.0) {
        return true;
    }
    return profile.displacement <= 0.0;
}

const char* ship_type_key(const ShipType type) {
    switch (type) {
    case ShipType::BulkCarrier:
        return "bulk_carrier";
    case ShipType::GasCarrierGte65k:
        return "gas_carrier_gte_65k";
    case ShipType::GasCarrierLt65k:
        return "gas_carrier_lt_65k";
    case ShipType::Tanker:
        return "tanker";
    case ShipType::ContainerShip:
        return "container_ship";
    case ShipType::GeneralCargo:
        return "general_cargo";
    case ShipType::RefrigeratedCargo:
        return "refrigerated_cargo";
    case ShipType::CombinationCarrier:
        return "combination_carrier";
    case ShipType::LngCarrierGte100k:
        return "lng_carrier_gte_100k";
    case ShipType::LngCarrierLt100k:
        return "lng_carrier_lt_100k";
    case ShipType::RoroVehicleCarrier:
        return "roro_vehicle_carrier";
    case ShipType::RoroCargo:
        return "roro_cargo";
    case ShipType::RoroPassenger:
        return "roro_passenger";
    case ShipType::CruisePassenger:
        return "cruise_passenger";
    case ShipType::Count:
        break;
    }

    throw std::invalid_argument("Unknown ship type");
}

ShipType ship_type_from_key(const std::string& value) {
    if (value == "bulk_carrier") {
        return ShipType::BulkCarrier;
    }
    if (value == "gas_carrier_gte_65k") {
        return ShipType::GasCarrierGte65k;
    }
    if (value == "gas_carrier_lt_65k") {
        return ShipType::GasCarrierLt65k;
    }
    if (value == "tanker") {
        return ShipType::Tanker;
    }
    if (value == "container_ship") {
        return ShipType::ContainerShip;
    }
    if (value == "general_cargo") {
        return ShipType::GeneralCargo;
    }
    if (value == "refrigerated_cargo") {
        return ShipType::RefrigeratedCargo;
    }
    if (value == "combination_carrier") {
        return ShipType::CombinationCarrier;
    }
    if (value == "lng_carrier_gte_100k") {
        return ShipType::LngCarrierGte100k;
    }
    if (value == "lng_carrier_lt_100k") {
        return ShipType::LngCarrierLt100k;
    }
    if (value == "roro_vehicle_carrier") {
        return ShipType::RoroVehicleCarrier;
    }
    if (value == "roro_cargo") {
        return ShipType::RoroCargo;
    }
    if (value == "roro_passenger") {
        return ShipType::RoroPassenger;
    }
    if (value == "cruise_passenger") {
        return ShipType::CruisePassenger;
    }

    throw std::invalid_argument("Unknown ship type setting: " + value);
}

const char* fuel_type_key(const FuelType type) {
    switch (type) {
    case FuelType::Hfo:
        return "hfo";
    case FuelType::Lfo:
        return "lfo";
    case FuelType::Mdo:
        return "mdo";
    case FuelType::Lng:
        return "lng";
    case FuelType::Methanol:
        return "methanol";
    }

    throw std::invalid_argument("Unknown fuel type");
}

FuelType fuel_type_from_key(const std::string& value) {
    if (value == "hfo") {
        return FuelType::Hfo;
    }
    if (value == "lfo") {
        return FuelType::Lfo;
    }
    if (value == "mdo") {
        return FuelType::Mdo;
    }
    if (value == "lng") {
        return FuelType::Lng;
    }
    if (value == "methanol") {
        return FuelType::Methanol;
    }

    throw std::invalid_argument("Unknown fuel type setting: " + value);
}

SettingsMap encode_profile_settings(const ProfileSettings& settings) {
    SettingsMap values;
    values[kProfileConfigured] = settings.has_profile ? "true" : "false";
    values[kName] = settings.profile.name;
    values[kImoNumber] = settings.profile.imo_number;
    values[kGt] = format_double(settings.profile.gt);
    values[kShipType] = ship_type_key(settings.profile.ship_type);
    values[kDwt] = format_double(settings.profile.dwt);
    values[kAdmiraltyCoefficient] = format_double(settings.profile.admiralty_coefficient);
    values[kSfoc] = format_double(settings.profile.sfoc);
    values[kFuelType] = fuel_type_key(settings.profile.fuel_type);
    values[kDisplacement] = format_double(settings.profile.displacement);
    values[kInstalledPowerMco] = format_double(settings.profile.installed_power_mco);
    values[kReferenceSpeed] = format_double(settings.profile.reference_speed);
    values[kSogThreshold] = format_double(settings.sog_threshold);
    values[kTargetYearOverride] = std::to_string(settings.target_year_override);
    values[kYtdSeedCo2Tonnes] = format_double(settings.ytd_seed_co2_tonnes);
    values[kYtdSeedDistanceNm] = format_double(settings.ytd_seed_distance_nm);
    return values;
}

ProfileSettings decode_profile_settings(const SettingsMap& values) {
    ProfileSettings settings;
    settings.has_profile = get_bool_or_default(values, kProfileConfigured, false);
    settings.profile.name = get_or_default(values, kName, "");
    settings.profile.imo_number = get_or_default(values, kImoNumber, "");
    settings.profile.gt = get_double_or_default(values, kGt, settings.profile.gt);
    settings.profile.ship_type =
        ship_type_from_key(get_or_default(values, kShipType, ship_type_key(settings.profile.ship_type)));
    settings.profile.dwt = get_double_or_default(values, kDwt, settings.profile.dwt);
    settings.profile.admiralty_coefficient =
        get_double_or_default(values, kAdmiraltyCoefficient, settings.profile.admiralty_coefficient);
    settings.profile.sfoc = get_double_or_default(values, kSfoc, settings.profile.sfoc);
    settings.profile.fuel_type =
        fuel_type_from_key(get_or_default(values, kFuelType, fuel_type_key(settings.profile.fuel_type)));
    settings.profile.displacement =
        get_double_or_default(values, kDisplacement, settings.profile.displacement);
    settings.profile.installed_power_mco =
        get_double_or_default(values, kInstalledPowerMco, settings.profile.installed_power_mco);
    settings.profile.reference_speed =
        get_double_or_default(values, kReferenceSpeed, settings.profile.reference_speed);
    settings.sog_threshold = get_double_or_default(values, kSogThreshold, settings.sog_threshold);
    settings.target_year_override =
        get_int_or_default(values, kTargetYearOverride, settings.target_year_override);
    settings.ytd_seed_co2_tonnes =
        get_double_or_default(values, kYtdSeedCo2Tonnes, settings.ytd_seed_co2_tonnes);
    settings.ytd_seed_distance_nm =
        get_double_or_default(values, kYtdSeedDistanceNm, settings.ytd_seed_distance_nm);
    return settings;
}

} // namespace eexi_cii
