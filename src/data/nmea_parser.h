#ifndef EEXI_CII_DATA_NMEA_PARSER_H
#define EEXI_CII_DATA_NMEA_PARSER_H

#include <string>

namespace eexi_cii {

struct RMCData {
    bool valid = false;
    double utc_seconds = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double sog_knots = -1.0;
    double cog_degrees = 0.0;
    int day = 0;
    int month = 0;
    int year = 0;
};

bool validate_checksum(const std::string& sentence);
RMCData parse_rmc(const std::string& sentence);

} // namespace eexi_cii

#endif
