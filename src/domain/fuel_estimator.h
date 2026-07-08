#ifndef EEXI_CII_DOMAIN_FUEL_ESTIMATOR_H
#define EEXI_CII_DOMAIN_FUEL_ESTIMATOR_H

#include "domain/vessel_profile.h"

namespace eexi_cii {

struct FuelEstimate {
    double shaft_power_kw = 0.0;
    double fuel_rate_tonnes_hr = 0.0;
    double co2_rate_tonnes_hr = 0.0;
    bool below_threshold = false;
};

FuelEstimate estimate_fuel(
    double sog_knots,
    const VesselProfile& profile,
    double sog_threshold = 1.0
);

} // namespace eexi_cii

#endif
