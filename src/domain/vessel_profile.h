#ifndef EEXI_CII_DOMAIN_VESSEL_PROFILE_H
#define EEXI_CII_DOMAIN_VESSEL_PROFILE_H

#include <string>

namespace eexi_cii {

enum class ShipType {
    BulkCarrier,
    GasCarrierGte65k,
    GasCarrierLt65k,
    Tanker,
    ContainerShip,
    GeneralCargo,
    RefrigeratedCargo,
    CombinationCarrier,
    LngCarrierGte100k,
    LngCarrierLt100k,
    RoroVehicleCarrier,
    RoroCargo,
    RoroPassenger,
    CruisePassenger,
    Count
};

enum class FuelType {
    Hfo,
    Lfo,
    Mdo,
    Lng,
    Methanol
};

bool uses_cgdist(ShipType type);
double emission_factor(FuelType fuel_type);

struct VesselProfile {
    std::string name;
    std::string imo_number;
    double gt = 0.0;
    ShipType ship_type = ShipType::BulkCarrier;
    double dwt = 0.0;
    double admiralty_coefficient = 0.0;
    double sfoc = 190.0;
    FuelType fuel_type = FuelType::Hfo;
    double displacement = 0.0;
    double installed_power_mco = 0.0;
    double reference_speed = 0.0;

    double cii_capacity() const;
};

} // namespace eexi_cii

#endif
