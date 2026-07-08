#ifndef EEXI_CII_DOMAIN_AER_ENGINE_H
#define EEXI_CII_DOMAIN_AER_ENGINE_H

#include "domain/cii_reference.h"
#include "domain/vessel_profile.h"

namespace eexi_cii {

struct AERResult {
    double aer = 0.0;
    char rating = 'E';
    double required_cii = 0.0;
    RatingBoundaries boundaries;
    bool uses_cgdist = false;
};

AERResult compute_aer(
    double total_co2_tonnes,
    double total_distance_nm,
    const VesselProfile& profile,
    int year
);

} // namespace eexi_cii

#endif
