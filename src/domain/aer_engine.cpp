#include "domain/aer_engine.h"

#include <stdexcept>

namespace eexi_cii {

AERResult compute_aer(
    const double total_co2_tonnes,
    const double total_distance_nm,
    const VesselProfile& profile,
    const int year
) {
    if (total_co2_tonnes < 0.0) {
        throw std::invalid_argument("Total CO2 cannot be negative");
    }
    if (total_distance_nm <= 0.0) {
        throw std::invalid_argument("Total distance must be greater than zero");
    }

    const double capacity = profile.cii_capacity();
    if (capacity <= 0.0) {
        throw std::invalid_argument("CII capacity must be greater than zero");
    }

    const double attained = total_co2_tonnes * 1000000.0 / (capacity * total_distance_nm);
    const double required = required_cii(profile.ship_type, capacity, year);
    const RatingBoundaries boundaries =
        rating_boundaries(profile.ship_type, capacity, year);

    return AERResult{
        attained,
        cii_rating(attained, profile.ship_type, capacity, year),
        required,
        boundaries,
        uses_cgdist(profile.ship_type)
    };
}

} // namespace eexi_cii
