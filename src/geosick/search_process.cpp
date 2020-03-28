#include <cassert>
#include <iostream>
#include "geosick/file_writer.hpp"
#include "geosick/geo_search.hpp"
#include "geosick/search_process.hpp"

namespace geosick {

SearchProcess::SearchProcess(const Config* cfg, const Sampler* sampler,
    const GeoSearch* search, const SickMap* sick_map, NotifyProcess* notify_proc)
: m_cfg(cfg), m_sampler(sampler), m_search(search),
  m_sick_map(sick_map), m_notify_proc(notify_proc)
{}


void SearchProcess::flush_user_rows() {
    m_sampler->sample(make_view(m_current_rows), m_current_samples);

    std::unordered_set<uint32_t> sick_ids;
    for (const auto& sample: m_current_samples) {
        m_search->find_users_within_circle(sample.lat, sample.lon,
            sample.accuracy_m, sample.time_index, sick_ids);
    }

    for (uint32_t sick_id: sick_ids) {
        MatchInput mi;
        mi.query_user_id = m_current_user_id;
        mi.query_rows = make_view(m_current_rows);
        mi.query_samples = make_view(m_current_samples);

        mi.sick_user_id = sick_id;
        size_t sick_idx = m_sick_map->user_id_to_idx.at(sick_id);
        mi.sick_rows = m_sick_map->rows_by_idx(sick_idx);
        mi.sick_samples = m_sick_map->samples_by_idx(sick_idx);
        
        MatchOutput mo = evaluate_match(*m_cfg, mi);
        m_notify_proc->notify(mi, mo);
    }

    m_current_samples.clear();
    m_current_rows.clear();
}

void SearchProcess::process_query_row(const GeoRow& row) {
    assert(row.user_id >= m_current_user_id);
    if (row.user_id != m_current_user_id) {
        this->flush_user_rows();
        m_current_user_id = row.user_id;
    }
    m_current_rows.push_back(row);
}

void SearchProcess::close() {
    this->flush_user_rows();
}

}
