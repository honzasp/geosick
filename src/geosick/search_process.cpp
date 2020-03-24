#include <cassert>
#include "geosick/geo_search.hpp"
#include "geosick/search_process.hpp"

namespace geosick {

using Hit = SearchProcess::Hit;
using HitHash = SearchProcess::HitHash;

SearchProcess::SearchProcess(const Sampler* sampler, const GeoSearch* search):
    m_sampler(sampler), m_search(search)
{}


void SearchProcess::flush_user_rows() {
    m_sampler->sample(make_view(m_current_rows), m_current_samples);
    for (const auto& sample: m_current_samples) {
        auto infected_ids = m_search->find_users_within_circle(
            sample.lat, sample.lon, sample.accuracy_m, sample.time_index);
        for (uint32_t infected_id: infected_ids) {
            m_hits.emplace(sample.user_id, infected_id);
        }
    }
    m_current_samples.clear();
    m_current_rows.clear();
}

void SearchProcess::process_row(const GeoRow& row) {
    assert(row.user_id >= m_current_user_id);
    if (row.user_id != m_current_user_id) {
        this->flush_user_rows();
        m_current_user_id = row.user_id;
    }
    m_current_rows.push_back(row);
}

void SearchProcess::process_end() {
    this->flush_user_rows();
}

std::unordered_set<Hit, HitHash> SearchProcess::read_hits() {
    return std::move(m_hits);
}

}
