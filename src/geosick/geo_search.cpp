#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include "geosick/geo_distance.hpp"
#include "geosick/geo_search.hpp"

namespace geosick {

GeoSearch::LatLonBins GeoSearch::get_bins(
    int32_t lat, int32_t lon, uint32_t radius) const
{
    double lat_e7 = (double)lat;
    double lon_e7 = (double)lon;
    double delta_lat_e7 = (double)radius * M_TO_DEG_E7;
    double delta_lon_e7 = (double)radius / std::cos(lat_e7*DEG_E7_TO_RAD) * M_TO_DEG_E7;

    int32_t lat_first = (int32_t)std::floor(lat_e7 - delta_lat_e7);
    int32_t lat_last = (int32_t)std::ceil(lat_e7 + delta_lat_e7);
    int32_t lon_first = (int32_t)std::floor(lon_e7 - delta_lon_e7);
    int32_t lon_last = (int32_t)std::ceil(lon_e7 + delta_lon_e7);

    return LatLonBins {
        .lat_first = lat_first / m_lat_delta,
        .lat_last = lat_last / m_lat_delta,
        .lon_first = lon_first / m_lon_delta,
        .lon_last = lon_last / m_lon_delta,
    };
}

uint32_t GeoSearch::get_hash(
    int32_t lat_bin, int32_t lon_bin, int32_t time_index) const
{
    // https://en.wikipedia.org/wiki/MurmurHash
    auto murmur_add = [](uint32_t h, uint32_t k) -> uint32_t {
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        h ^= k;
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
        return h;
    };

    uint32_t h = 0x8d0e03f0;
    h = murmur_add(h, uint32_t(lat_bin));
    h = murmur_add(h, uint32_t(lon_bin));
    h = murmur_add(h, uint32_t(time_index));

    h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
    return h;
}


GeoSearch::GeoSearch(const Config& cfg, ArrayView<const GeoSample> samples) {
    m_lat_delta = (int32_t)std::ceil(
        cfg.search.bin_delta_m * M_TO_DEG_E7);
    m_lon_delta = (int32_t)std::ceil(
        cfg.search.bin_delta_m * M_TO_DEG_E7 * std::cos(MEAN_LAT_E7*DEG_E7_TO_RAD));
    m_bucket_count = cfg.search.bucket_count;

    std::vector<std::vector<UserPoint>> buckets(m_bucket_count);
    size_t point_count = 0;
    for (const auto& sample: samples) {
        auto bins = this->get_bins(sample.lat, sample.lon, sample.accuracy_m);
        for (int32_t i = bins.lat_first; i <= bins.lat_last; ++i) {
            for (int32_t j = bins.lon_first; j <= bins.lon_last; ++j) {
                uint32_t hash = this->get_hash(i, j, sample.time_index);
                size_t bucket_idx = hash % buckets.size();
                buckets.at(bucket_idx).push_back(UserPoint {
                    .time_index = sample.time_index,
                    .lat = sample.lat,
                    .lon = sample.lon,
                    .hash = hash,
                    .radius_m = sample.accuracy_m,
                    .user_id = sample.user_id,
                });
                ++point_count;
            }
        }
    }

    m_buckets.reserve(m_bucket_count+1);
    m_points.reserve(point_count);
    for (auto& bucket: buckets) {
        std::sort(bucket.begin(), bucket.end(),
            [&](const UserPoint& p1, const UserPoint& p2)
        {
            return p1.time_index < p2.time_index;
        });

        m_buckets.push_back(m_points.size());
        m_points.insert(m_points.end(), bucket.begin(), bucket.end());
    }
    m_buckets.push_back(m_points.size());

    std::cout << "  built search structure of " << point_count << " points "
        "from " << samples.size() << " samples" << std::endl;
}

void GeoSearch::find_users_in_bin(int32_t lat, int32_t lon, uint32_t radius_m,
    int32_t time_index, int32_t lat_bin, int32_t lon_bin,
    std::unordered_set<uint32_t>& out_user_ids) const
{
    uint32_t hash = this->get_hash(lat_bin, lon_bin, time_index);
    size_t bucket_idx = hash % m_bucket_count;
    auto [begin, end] = this->find_time_range_in_bucket(bucket_idx, time_index);
    for (size_t i = begin; i < end; ++i) {
        const auto& point = m_points.at(i);
        assert(point.time_index == time_index);
        m_point_hit_count.fetch_add(1);
        if (point.hash != hash) { continue; }

        m_point_test_count.fetch_add(1);
        double distance_pow2 = pow2_geo_distance_fast_m(
            point.lat, point.lon, lat, lon);
        double max_distance = (double)radius_m + (double)point.radius_m;
        if (distance_pow2 > max_distance*max_distance) { continue; }

        m_point_pass_count.fetch_add(1);
        out_user_ids.insert(point.user_id);
    }
    m_bin_hit_count.fetch_add(1);
}

std::pair<size_t,size_t> GeoSearch::find_time_range_in_bucket(
    size_t bucket_idx, int32_t time_index) const
{
    size_t bucket_begin = m_buckets.at(bucket_idx);
    size_t bucket_end = m_buckets.at(bucket_idx + 1);
    while (bucket_begin < bucket_end) {
        size_t bucket_mid = bucket_begin + (bucket_end - bucket_begin) / 2;
        int32_t mid_time_index = m_points.at(bucket_mid).time_index;
        if (mid_time_index < time_index) {
            bucket_begin = bucket_mid + 1;
        } else if (mid_time_index > time_index) {
            bucket_end = bucket_mid;
        } else {
            size_t begin = bucket_mid;
            for (; begin > bucket_begin 
                && m_points.at(begin-1).time_index == time_index; --begin) {}
            size_t end = bucket_mid + 1;
            for (; end + 1 < bucket_end 
                && m_points.at(end+1).time_index == time_index; ++end) {}
            return {begin, end};
        }
    }
    return {0, 0};
}

void GeoSearch::find_users_within_circle(int32_t lat, int32_t lon,
    uint32_t radius_m, int32_t time_index,
    std::unordered_set<uint32_t>& out_user_ids) const
{
    auto bins = this->get_bins(lat, lon, radius_m);
    for (int32_t i = bins.lat_first; i <= bins.lat_last; ++i) {
        for (int32_t j = bins.lon_first; j <= bins.lon_last; ++j) {
            this->find_users_in_bin(lat, lon, radius_m, time_index,
                i, j, out_user_ids);
        }
    }
    m_query_count.fetch_add(1);
}

void GeoSearch::close() {
    std::cout << "Search structure stats:" << std::endl
        << "  queries: " << m_query_count.load() << std::endl
        << "  bin hits: " << m_bin_hit_count.load() << std::endl
        << "  point hits: " << m_point_hit_count.load() << std::endl
        << "  point tests: " << m_point_test_count.load() << std::endl
        << "  point passes: " << m_point_pass_count.load() << std::endl;
}

}
