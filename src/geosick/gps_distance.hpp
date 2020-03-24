#pragma once
#include <cstdint>

namespace geosick {

double gps_distance_haversine_m(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2);
double pow2_gps_distance_fast_m(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2);

}
