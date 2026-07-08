#include "data/haversine.h"

#include <algorithm>
#include <cmath>

namespace eexi_cii {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthRadiusNm = 3440.065;

double radians(const double degrees) {
    return degrees * kPi / 180.0;
}

} // namespace

double haversine_nm(
    const double lat1,
    const double lon1,
    const double lat2,
    const double lon2
) {
    const double delta_lat = radians(lat2 - lat1);
    const double delta_lon = radians(lon2 - lon1);
    const double rlat1 = radians(lat1);
    const double rlat2 = radians(lat2);

    const double sin_lat = std::sin(delta_lat / 2.0);
    const double sin_lon = std::sin(delta_lon / 2.0);
    const double a = sin_lat * sin_lat +
                     std::cos(rlat1) * std::cos(rlat2) * sin_lon * sin_lon;
    const double clamped_a = std::clamp(a, 0.0, 1.0);
    const double c = 2.0 * std::atan2(std::sqrt(clamped_a), std::sqrt(1.0 - clamped_a));

    return kEarthRadiusNm * c;
}

} // namespace eexi_cii
