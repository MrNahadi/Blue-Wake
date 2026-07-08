#include "domain/cii_reference.h"

#include <cmath>
#include <stdexcept>

namespace eexi_cii {

CIICoefficients cii_coefficients(const ShipType type) {
    switch (type) {
    case ShipType::BulkCarrier:
        // MEPC.353(78) G2 reference line; MEPC.354(78) G4 rating vectors.
        return CIICoefficients{4745.0, 0.622, 0.86, 0.94, 1.06, 1.18};
    default:
        throw std::invalid_argument(
            "CII coefficients for this ship type are not implemented yet"
        );
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

    const CIICoefficients coeffs = cii_coefficients(type);
    const double reference = coeffs.a * std::pow(capacity, -coeffs.c);
    const double reduction = reduction_factor_z(year);

    return reference * (1.0 - reduction / 100.0);
}

RatingBoundaries rating_boundaries(
    const ShipType type,
    const double capacity,
    const int year
) {
    const CIICoefficients coeffs = cii_coefficients(type);
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
