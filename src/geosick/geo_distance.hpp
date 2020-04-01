#pragma once
#include <cstdint>

namespace geosick {

static const double DEG_E7_TO_M = 0.011119508;
static const double M_TO_DEG_E7 = 89.93204;
static const double DEG_E7_TO_RAD = 1.745329251994e-9;
static const double MEAN_LAT_E7 = 48e7;
static const double MEAN_EARTH_RADIUS_M = 6371009.0;

double geo_distance_haversine_m(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2);
double pow2_geo_distance_fast_m(int32_t lat1, int32_t lon1, int32_t lat2, int32_t lon2);

}
