#pragma once
#include <unordered_set>
#include "geosick/sampler.hpp"

namespace geosick {

struct GeoSearch;

class SearchProcess {
public:
    struct Hit {
        uint32_t healthy_user_id;
        uint32_t infected_user_id;

        Hit(uint32_t healthy_user_id, uint32_t infected_user_id):
            healthy_user_id(healthy_user_id), infected_user_id(infected_user_id) {}

        friend bool operator==(const Hit& h1, const Hit& h2) {
            return h1.healthy_user_id == h2.healthy_user_id &&
                h1.infected_user_id == h2.infected_user_id;
        }
    };

    struct HitHash {
        size_t operator()(const Hit& hit) const {
            size_t h = (size_t)hit.healthy_user_id;
            h ^= ((size_t)hit.infected_user_id) * 4093;
            return h;
        }
    };

private:
    const Sampler* m_sampler;
    const GeoSearch* m_search;

    uint32_t m_current_user_id = 0;
    std::vector<GeoRow> m_current_rows;
    std::vector<GeoSample> m_current_samples;
    std::unordered_set<Hit, HitHash> m_hits;

    void flush_user_rows();

public:
    SearchProcess(const Sampler* sampler, const GeoSearch* search);
    void process_row(const GeoRow& row);
    void process_end();

    std::unordered_set<Hit, HitHash> read_hits();
};

}
