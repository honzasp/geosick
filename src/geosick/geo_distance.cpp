#include <cmath>
#include "geosick/geo_distance.hpp"

namespace geosick {

static double pow2(double x) {
    return x*x;
}

double geo_distance_haversine_m(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2) {
    double lat_rad1 = double(lat1) * DEG_E7_TO_RAD;
    double lon_rad1 = double(lon1) * DEG_E7_TO_RAD;
    double lat_rad2 = double(lat2) * DEG_E7_TO_RAD;
    double lon_rad2 = double(lon2) * DEG_E7_TO_RAD;
	double delta_rad = std::asin(std::sqrt(
        pow2(std::sin((lat_rad2 - lat_rad1) * 0.5)) +
        pow2(std::sin((lon_rad2 - lon_rad1) * 0.5)) *
            std::cos(lat_rad1) * std::cos(lat_rad2)
    ));
	return 2.0*MEAN_EARTH_RADIUS_M * delta_rad;
}

double pow2_geo_distance_fast_m(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2) {
    double lat_rad = double((lat1 + lat2)/2) * DEG_E7_TO_RAD;
    double northing_m = double(lat2 - lat1) * DEG_E7_TO_M;
    double easting_m = double(lon2 - lon1) * DEG_E7_TO_M * std::cos(lat_rad);
    return pow2(northing_m) + pow2(easting_m);
}

}
