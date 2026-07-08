#ifndef EEXI_CII_EXPORT_CSV_EXPORTER_H
#define EEXI_CII_EXPORT_CSV_EXPORTER_H

#include "data/accumulator.h"
#include "domain/aer_engine.h"
#include "domain/vessel_profile.h"

#include <string>
#include <vector>

namespace eexi_cii {

std::string export_annual_csv(
    const YTDState& ytd,
    const VesselProfile& profile,
    const AERResult& aer_result
);

std::string export_voyage_csv(const std::vector<VoyageRecord>& records);

} // namespace eexi_cii

#endif
