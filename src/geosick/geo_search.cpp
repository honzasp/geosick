#include "geosick/geo_search.hpp"

#define _USE_MATH_DEFINES
#include <math.h>
#include <cassert>

namespace geosick {
namespace {

int32_t pow2(int32_t x) {
    return x*x;
}

double pow2(double x) {
    return x*x;
}

double degree_to_radian(int32_t angle_int) {
    static constexpr int32_t e7 = 10000000; // 10^7
    double angle = double(angle_int) / e7;
    return M_PI * angle / 180.0;
}

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

/*
bool
circles_intersect(int32_t lat1, int32_t lon1, int32_t r1, int32_t lat2, int32_t lon2, int32_t r2)
{
    return gps_distance_haversine_m(lat1, lon1, lat2, lon2) <= r1 + r2;
}
*/

bool
circles_intersect_fast(int32_t lat1, int32_t lon1, int32_t r1, int32_t lat2, int32_t lon2, int32_t r2)
{
    return pow2_gps_distance_fast_m(lat1, lon1, lat2, lon2) <= pow2(r1 + r2);
}

} // END OF ANONYMOUS NAMESPACE


GeoSearch::GeoSearch(const std::vector<GeoSample>& samples)
 : m_lat_delta(get_lat_delta()), m_lon_delta(get_lon_delta())
{
    m_points.reserve(samples.size());
    for (const auto& sample: samples) {
        insert_sample(sample);
    }
}

std::vector<GeoSample::UserID>
GeoSearch::find_users_within_circle(int32_t lat, int32_t lon, unsigned radius, TimeIdx time_index) const
{
    SectorKey key{time_index, get_lat_key(lat), get_lon_key(lon)};
    auto it = m_sectors.find(key);
    if (it == m_sectors.end()) {
        return {};
    }

    std::vector<GeoSample::UserID> users;
    for (const auto& p: it->second) {
        if (circles_intersect_fast(lat, lon, radius, p.lat, p.lon, p.accuracy_m)) {
            users.push_back(p.user_id);
        }
    }
    return users;
}

void
GeoSearch::insert_sample(const GeoSample& sample)
{
    UserGeoPoint geo_point{
        .user_id = sample.user_id,
        .lat = sample.lat,
        .lon = sample.lon,
        .accuracy_m = sample.accuracy_m,
    };
    auto lat_key = get_lat_key(sample.lat);
    auto lon_key = get_lon_key(sample.lon);

    for (auto lat_shift: {-1, 0, 1}) {
        for (auto lon_shift: {-1, 0, 1}) {
            SectorKey key{sample.time_index, lat_key + lat_shift, lon_key + lon_shift};
            insert_geo_point(key, geo_point);
        }
    }
}

void
GeoSearch::insert_geo_point(const SectorKey& key, const UserGeoPoint& point)
{
    if (auto it = m_sectors.find(key); it != m_sectors.end()) {
        m_sectors[key].push_back(point);
    } else {
        m_sectors[key] = {point};
    }
}

int32_t
GeoSearch::get_lat_key(int32_t lat) const
{
    return lat / m_lat_delta;
}

int32_t
GeoSearch::get_lon_key(int32_t lon) const
{
    return lon / m_lon_delta;
}

int32_t
GeoSearch::get_lat_delta() {
    static constexpr int32_t lat1 = 491000000;
    static constexpr int32_t lon1 = 165000000;
    static constexpr int32_t lat2 = lat1;
    static constexpr int32_t lon2 = 166000000;
    auto angle_per_meter = abs(lon1 - lon2) / gps_distance_haversine_m(lat1, lon1, lat2, lon2);
    return angle_per_meter * GPS_HASH_PRECISION_M;
}

int32_t
GeoSearch::get_lon_delta() {
    static constexpr int32_t lat1 = 491000000;
    static constexpr int32_t lon1 = 165000000;
    static constexpr int32_t lat2 = 492000000;
    static constexpr int32_t lon2 = lon1;
    auto angle_per_meter = abs(lat1 - lat2) / gps_distance_haversine_m(lat1, lon1, lat2, lon2);
    return angle_per_meter * GPS_HASH_PRECISION_M;
}

}
