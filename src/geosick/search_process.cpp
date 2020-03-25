#include <cassert>
#include "geosick/file_writer.hpp"
#include "geosick/geo_search.hpp"
#include "geosick/search_process.hpp"

namespace geosick {

using Hit = SearchProcess::Hit;
using HitHash = SearchProcess::HitHash;

SearchProcess::SearchProcess(const Sampler* sampler,
    const GeoSearch* search, FileWriter* writer,
    const std::unordered_set<uint32_t>* sick_user_ids):
    m_sampler(sampler), m_search(search),
    m_writer(writer), m_sick_user_ids(sick_user_ids) {}


void SearchProcess::flush_user_rows() {
    m_user_offsets.emplace_back(m_current_user_id, m_writer->get_offset());
    m_writer->write(make_view(m_current_rows));

    if (!m_sick_user_ids->count(m_current_user_id)) {
        m_sampler->sample(make_view(m_current_rows), m_current_samples);
        for (const auto& sample: m_current_samples) {
            auto sick_ids = m_search->find_users_within_circle(
                sample.lat, sample.lon, sample.accuracy_m, sample.time_index);
            for (uint32_t sick_id: sick_ids) {
                m_hits.emplace(sample.user_id, sick_id);
            }
        }
        m_current_samples.clear();
    }

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
    m_user_offsets.emplace_back(UINT32_MAX, m_writer->get_offset());
    m_writer->flush();
}

std::unordered_set<Hit, HitHash> SearchProcess::read_hits() {
    return std::move(m_hits);
}

std::vector<GeoRow> SearchProcess::read_user_rows(uint32_t user_id) {
    size_t idx_begin = 0;
    size_t idx_end = m_user_offsets.size() - 1;
    while (idx_begin < idx_end) {
        size_t idx_mid = idx_begin + (idx_end - idx_begin) / 2;
        if (m_user_offsets[idx_mid].user_id < user_id) {
            idx_begin = idx_mid + 1;
        } else if (m_user_offsets[idx_mid].user_id > user_id) {
            idx_end = idx_mid;
        } else {
            size_t offset_begin = m_user_offsets.at(idx_mid).offset;
            size_t offset_end = m_user_offsets.at(idx_mid + 1).offset;
            size_t row_count = (offset_end - offset_begin) / sizeof(GeoRow);

            std::vector<GeoRow> rows(row_count);
            m_writer->pread(offset_begin, make_view(rows));
            return rows;
        }
    }
    throw std::runtime_error("User with the given id was not found");
}

}
