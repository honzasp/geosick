#pragma once
#include "geosick/sampler.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <boost/functional/hash.hpp>

namespace geosick {

class GeoSearch {
public:
    using TimeIdx = uint32_t;

private:
    static constexpr double GPS_HASH_PRECISION_M = 200;
    const int32_t m_lat_delta;
    const int32_t m_lon_delta;

    struct UserGeoPoint {
        GeoSample::UserID user_id;
        int32_t lat, lon;
        uint16_t accuracy_m;
    };
    std::vector<UserGeoPoint> m_points;
    using SectorKey = std::tuple<TimeIdx, int32_t, int32_t>;
    std::unordered_map<SectorKey, std::vector<UserGeoPoint>, boost::hash<SectorKey>> m_sectors;

public:

    explicit GeoSearch(const std::vector<GeoSample>& samples);

    void find_users_within_circle(int32_t lat, int32_t lon,
        uint32_t radius_m, TimeIdx time_index,
        std::unordered_set<uint32_t>& out_user_ids) const;

private:
    void insert_sample(const GeoSample& sample);
    void insert_geo_point(const SectorKey& key, const UserGeoPoint& point);

    int32_t get_lat_key(int32_t lat) const;
    int32_t get_lon_key(int32_t lon) const;

    static int32_t get_lat_delta();
    static int32_t get_lon_delta();

};

}
