#pragma once
#include <unordered_set>
#include "geosick/config.hpp"
#include "geosick/sampler.hpp"

namespace geosick {

class GeoSearch {
    struct UserPoint {
        int32_t time_index;
        int32_t lat, lon;
        uint32_t hash;
        uint32_t radius_m;
        uint32_t user_id;
    };

    struct LatLonBins {
        int32_t lat_first;
        int32_t lat_last;
        int32_t lon_first;
        int32_t lon_last;
    };

    int32_t m_lat_delta;
    int32_t m_lon_delta;
    size_t m_bucket_count;
    std::vector<UserPoint> m_points;
    std::vector<size_t> m_buckets;

    LatLonBins get_bins(int32_t lat, int32_t lon, uint32_t radius) const;
    uint32_t get_hash(int32_t lat_bin, int32_t lon_bin, int32_t time_index) const;

    void find_users_in_bin(int32_t lat, int32_t lon, uint32_t radius_m,
        int32_t time_index, int32_t lat_bin, int32_t lon_bin,
        std::unordered_set<uint32_t>& out_user_ids) const;
    std::pair<size_t, size_t> find_time_range_in_bucket(
        size_t bucket_idx, int32_t time_index) const;
public:
    explicit GeoSearch(const Config& cfg, ArrayView<const GeoSample> samples);

    void find_users_within_circle(int32_t lat, int32_t lon, uint32_t radius_m,
        int32_t time_index, std::unordered_set<uint32_t>& out_user_ids) const;
};

}
