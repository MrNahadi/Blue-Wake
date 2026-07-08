#ifndef EEXI_CII_DATA_PERSISTENCE_H
#define EEXI_CII_DATA_PERSISTENCE_H

#include "data/accumulator.h"

#include <string>

namespace eexi_cii {

std::string serialize_accumulator(const Accumulator& accumulator);
Accumulator deserialize_accumulator(const std::string& json);

void save_accumulator(const std::string& filepath, const Accumulator& accumulator);
Accumulator load_accumulator(const std::string& filepath);

} // namespace eexi_cii

#endif
