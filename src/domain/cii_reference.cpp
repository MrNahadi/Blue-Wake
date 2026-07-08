#include "domain/cii_reference.h"

#include <cmath>
#include <stdexcept>

namespace eexi_cii {

namespace {

CIICoefficients with_vectors(
    const double a,
    const double c,
    const double d1,
    const double d2,
    const double d3,
    const double d4
) {
    return CIICoefficients{a, c, d1, d2, d3, d4};
}

} // namespace

CIICoefficients cii_coefficients(const ShipType type, const double capacity) {
    if (capacity <= 0.0) {
        throw std::invalid_argument("CII capacity must be greater than zero");
    }

    switch (type) {
    case ShipType::BulkCarrier:
        // MEPC.353(78) G2 reference line; MEPC.354(78) G4 rating vectors.
        return with_vectors(4745.0, 0.622, 0.86, 0.94, 1.06, 1.18);
    case ShipType::GasCarrierGte65k:
        return with_vectors(14405e7, 2.071, 0.81, 0.91, 1.12, 1.44);
    case ShipType::GasCarrierLt65k:
        return with_vectors(8104.0, 0.639, 0.85, 0.95, 1.06, 1.25);
    case ShipType::Tanker:
        return with_vectors(5247.0, 0.610, 0.82, 0.93, 1.08, 1.28);
    case ShipType::ContainerShip:
        return with_vectors(1984.0, 0.489, 0.83, 0.94, 1.07, 1.19);
    case ShipType::GeneralCargo:
        if (capacity >= 20000.0) {
            return with_vectors(31948.0, 0.792, 0.83, 0.94, 1.06, 1.19);
        }
        return with_vectors(588.0, 0.3885, 0.83, 0.94, 1.06, 1.19);
    case ShipType::RefrigeratedCargo:
        return with_vectors(4600.0, 0.557, 0.78, 0.91, 1.07, 1.20);
    case ShipType::CombinationCarrier:
        return with_vectors(5119.0, 0.622, 0.87, 0.96, 1.06, 1.14);
    case ShipType::LngCarrierGte100k:
        return with_vectors(9.827, 0.0, 0.89, 0.98, 1.06, 1.13);
    case ShipType::LngCarrierLt100k:
        return with_vectors(
            capacity >= 65000.0 ? 14479e10 : 14779e10,
            2.673,
            0.78,
            0.92,
            1.10,
            1.37
        );
    case ShipType::RoroVehicleCarrier:
        if (capacity < 30000.0) {
            return with_vectors(330.0, 0.329, 0.86, 0.94, 1.06, 1.16);
        }
        return with_vectors(3627.0, 0.590, 0.86, 0.94, 1.06, 1.16);
    case ShipType::RoroCargo:
        return with_vectors(1967.0, 0.485, 0.76, 0.89, 1.08, 1.27);
    case ShipType::RoroPassenger:
        return with_vectors(2023.0, 0.460, 0.76, 0.92, 1.14, 1.30);
    case ShipType::CruisePassenger:
        return with_vectors(930.0, 0.383, 0.87, 0.95, 1.06, 1.16);
    case ShipType::Count:
        break;
    }

    throw std::invalid_argument("Unknown ship type");
}

double cii_reference_capacity(const ShipType type, const double capacity) {
    if (capacity <= 0.0) {
        throw std::invalid_argument("CII capacity must be greater than zero");
    }

    switch (type) {
    case ShipType::BulkCarrier:
        return capacity >= 279000.0 ? 279000.0 : capacity;
    case ShipType::LngCarrierLt100k:
        return capacity < 65000.0 ? 65000.0 : capacity;
    case ShipType::RoroVehicleCarrier:
        return capacity >= 57700.0 ? 57700.0 : capacity;
    case ShipType::GasCarrierGte65k:
    case ShipType::GasCarrierLt65k:
    case ShipType::Tanker:
    case ShipType::ContainerShip:
    case ShipType::GeneralCargo:
    case ShipType::RefrigeratedCargo:
    case ShipType::CombinationCarrier:
    case ShipType::LngCarrierGte100k:
    case ShipType::RoroCargo:
    case ShipType::RoroPassenger:
    case ShipType::CruisePassenger:
        return capacity;
    default:
        throw std::invalid_argument("Unknown ship type");
    }
}

double reduction_factor_z(const int year) {
    switch (year) {
    case 2023:
        return 5.0;
    case 2024:
        return 7.0;
    case 2025:
        return 9.0;
    case 2026:
        return 11.0;
    default:
        throw std::invalid_argument("Reduction factor is only source-tagged for 2023-2026");
    }
}

double required_cii(const ShipType type, const double capacity, const int year) {
    if (capacity <= 0.0) {
        throw std::invalid_argument("CII capacity must be greater than zero");
    }

    const double reference_capacity = cii_reference_capacity(type, capacity);
    const CIICoefficients coeffs = cii_coefficients(type, capacity);
    const double reference = coeffs.a * std::pow(reference_capacity, -coeffs.c);
    const double reduction = reduction_factor_z(year);

    return reference * (1.0 - reduction / 100.0);
}

RatingBoundaries rating_boundaries(
    const ShipType type,
    const double capacity,
    const int year
) {
    const CIICoefficients coeffs = cii_coefficients(type, capacity);
    const double required = required_cii(type, capacity, year);

    return RatingBoundaries{
        required * coeffs.d1,
        required * coeffs.d2,
        required * coeffs.d3,
        required * coeffs.d4
    };
}

char cii_rating(
    const double attained,
    const ShipType type,
    const double capacity,
    const int year
) {
    if (attained < 0.0) {
        throw std::invalid_argument("Attained CII cannot be negative");
    }

    const RatingBoundaries boundaries = rating_boundaries(type, capacity, year);

    if (attained <= boundaries.ab) {
        return 'A';
    }
    if (attained <= boundaries.bc) {
        return 'B';
    }
    if (attained <= boundaries.cd) {
        return 'C';
    }
    if (attained <= boundaries.de) {
        return 'D';
    }

    return 'E';
}

} // namespace eexi_cii
