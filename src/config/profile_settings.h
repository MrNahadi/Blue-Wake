#ifndef EEXI_CII_CONFIG_PROFILE_SETTINGS_H
#define EEXI_CII_CONFIG_PROFILE_SETTINGS_H

#include "domain/vessel_profile.h"

#include <map>
#include <string>

namespace eexi_cii {

using SettingsMap = std::map<std::string, std::string>;

struct ProfileSettings {
    VesselProfile profile;
    double sog_threshold = 1.0;
    int target_year_override = 0;
    double ytd_seed_co2_tonnes = 0.0;
    double ytd_seed_distance_nm = 0.0;
    bool has_profile = false;

    bool setup_required() const;
};

const char* ship_type_key(ShipType type);
ShipType ship_type_from_key(const std::string& value);

const char* fuel_type_key(FuelType type);
FuelType fuel_type_from_key(const std::string& value);

SettingsMap encode_profile_settings(const ProfileSettings& settings);
ProfileSettings decode_profile_settings(const SettingsMap& values);

} // namespace eexi_cii

#endif
