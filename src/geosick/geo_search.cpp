#include "geosick/geo_search.hpp"
#include "geosick/geo_distance.hpp"

#define _USE_MATH_DEFINES
#include <math.h>
#include <cassert>

namespace geosick {
namespace {

uint32_t pow2(uint32_t x) {
    return x*x;
}


/*
bool
circles_intersect(int32_t lat1, int32_t lon1, int32_t r1, int32_t lat2, int32_t lon2, int32_t r2)
{
    return geo_distance_haversine_m(lat1, lon1, lat2, lon2) <= r1 + r2;
}
*/

bool
circles_intersect_fast(int32_t lat1, int32_t lon1, uint32_t r1, int32_t lat2, int32_t lon2, uint32_t r2)
{
    return pow2_geo_distance_fast_m(lat1, lon1, lat2, lon2) <= pow2(r1 + r2);
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

void GeoSearch::find_users_within_circle(int32_t lat, int32_t lon,
    uint32_t radius, TimeIdx time_index,
    std::unordered_set<uint32_t>& out_user_ids) const
{
    SectorKey key{time_index, get_lat_key(lat), get_lon_key(lon)};
    auto it = m_sectors.find(key);
    if (it == m_sectors.end()) { return; }

    for (const auto& p: it->second) {
        if (circles_intersect_fast(lat, lon, radius, p.lat, p.lon, p.accuracy_m)) {
            out_user_ids.insert(p.user_id);
        }
    }
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
        for (auto lon_shift: {-1, 0, 1}) { // TODO: This is maybe too much memory waste.
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
    auto angle_per_meter = abs(lon1 - lon2) / geo_distance_haversine_m(lat1, lon1, lat2, lon2);
    return int32_t(angle_per_meter * GPS_HASH_PRECISION_M);
}

int32_t
GeoSearch::get_lon_delta() {
    static constexpr int32_t lat1 = 491000000;
    static constexpr int32_t lon1 = 165000000;
    static constexpr int32_t lat2 = 492000000;
    static constexpr int32_t lon2 = lon1;
    auto angle_per_meter = abs(lat1 - lat2) / geo_distance_haversine_m(lat1, lon1, lat2, lon2);
    return int32_t(angle_per_meter * GPS_HASH_PRECISION_M);
}

}
