#include "domain/fuel_estimator.h"

#include <cmath>
#include <stdexcept>

namespace eexi_cii {

FuelEstimate estimate_fuel(
    const double sog_knots,
    const VesselProfile& profile,
    const double sog_threshold
) {
    if (sog_threshold < 0.0) {
        throw std::invalid_argument("SOG threshold cannot be negative");
    }

    if (sog_knots < sog_threshold) {
        return FuelEstimate{0.0, 0.0, 0.0, true};
    }

    if (profile.displacement <= 0.0) {
        throw std::invalid_argument("Displacement must be greater than zero");
    }

    if (profile.admiralty_coefficient <= 0.0) {
        throw std::invalid_argument("Admiralty Coefficient must be greater than zero");
    }

    if (profile.sfoc < 0.0) {
        throw std::invalid_argument("SFOC cannot be negative");
    }

    const double displacement_term = std::pow(profile.displacement, 2.0 / 3.0);
    const double speed_term = std::pow(sog_knots, 3.0);
    const double shaft_power_kw =
        displacement_term * speed_term / profile.admiralty_coefficient;
    const double fuel_rate_tonnes_hr = shaft_power_kw * profile.sfoc / 1000000.0;
    const double co2_rate_tonnes_hr = fuel_rate_tonnes_hr * emission_factor(profile.fuel_type);

    return FuelEstimate{
        shaft_power_kw,
        fuel_rate_tonnes_hr,
        co2_rate_tonnes_hr,
        false
    };
}

} // namespace eexi_cii
