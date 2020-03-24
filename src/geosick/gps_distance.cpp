#include "geosick/gps_distance.hpp"

#define _USE_MATH_DEFINES
#include <math.h>

namespace geosick {
namespace {

double pow2(double x) {
    return x*x;
}

double degree_to_radian(int32_t angle_int) {
    static constexpr int32_t e7 = 10000000; // 10^7
    double angle = double(angle_int) / e7;
    return M_PI * angle / 180.0;
}

} // END OF ANONYMOUS NAMESPACE

double gps_distance_haversine_m(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2) {
    static constexpr double earth_radius_m = 6372.8 * 1000;

    double lat_rad1 = degree_to_radian(lat1);
	double lon_rad1 = degree_to_radian(lon1);
	double lat_rad2 = degree_to_radian(lat2);
	double lon_rad2 = degree_to_radian(lon2);

	double diff_la = lat_rad2 - lat_rad1;
	double diff_lo = lon_rad2 - lon_rad1;

	double computation = asin(
        sqrt(sin(diff_la / 2) * sin(diff_la / 2)
        + cos(lat_rad1) * cos(lat_rad2) * sin(diff_lo / 2) * sin(diff_lo / 2))
    );
	return 2 * earth_radius_m * computation;
}

double pow2_gps_distance_fast_m(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2) {
    static constexpr double lat_deg_e7_to_m = 0.0111132; // at latitude 45 deg, negligible variation with latitude
    double lat_rad = lat2 / 174533.;
    double lon_deg_e7_to_m = 0.0111319 * cos(lat_rad);

    double northing_m = (lat2 - lat1) * lat_deg_e7_to_m;
    double easting_m = (lon2 - lon1) * lon_deg_e7_to_m;
    return pow2(northing_m) + pow2(easting_m);
}

}
