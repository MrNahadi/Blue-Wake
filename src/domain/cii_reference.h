#ifndef EEXI_CII_DOMAIN_CII_REFERENCE_H
#define EEXI_CII_DOMAIN_CII_REFERENCE_H

#include "domain/vessel_profile.h"

namespace eexi_cii {

struct CIICoefficients {
    double a = 0.0;
    double c = 0.0;
    double d1 = 0.0;
    double d2 = 0.0;
    double d3 = 0.0;
    double d4 = 0.0;
};

struct RatingBoundaries {
    double ab = 0.0;
    double bc = 0.0;
    double cd = 0.0;
    double de = 0.0;
};

CIICoefficients cii_coefficients(ShipType type);
double reduction_factor_z(int year);
double required_cii(ShipType type, double capacity, int year);
RatingBoundaries rating_boundaries(ShipType type, double capacity, int year);
char cii_rating(double attained, ShipType type, double capacity, int year);

} // namespace eexi_cii

#endif
