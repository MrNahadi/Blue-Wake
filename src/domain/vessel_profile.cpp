#include "domain/vessel_profile.h"

#include <stdexcept>

namespace eexi_cii {

bool uses_cgdist(const ShipType type) {
    switch (type) {
    case ShipType::RoroVehicleCarrier:
    case ShipType::RoroCargo:
    case ShipType::RoroPassenger:
    case ShipType::CruisePassenger:
        return true;
    default:
        return false;
    }
}

double emission_factor(const FuelType fuel_type) {
    switch (fuel_type) {
    case FuelType::Hfo:
        return 3.114;
    case FuelType::Lfo:
        return 3.151;
    case FuelType::Mdo:
        return 3.206;
    case FuelType::Lng:
        return 2.750;
    case FuelType::Methanol:
        return 1.375;
    }

    throw std::invalid_argument("Unknown fuel type");
}

double VesselProfile::cii_capacity() const {
    return uses_cgdist(ship_type) ? gt : dwt;
}

} // namespace eexi_cii
