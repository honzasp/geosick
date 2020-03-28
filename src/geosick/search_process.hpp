#pragma once
#include <unordered_set>
#include "geosick/notify_process.hpp"
#include "geosick/sampler.hpp"
#include "geosick/sick_map.hpp"

namespace geosick {

class FileWriter;
class GeoSearch;

class SearchProcess {
    const Config* m_cfg;
    const Sampler* m_sampler;
    const GeoSearch* m_search;
    const SickMap* m_sick_map;
    NotifyProcess* m_notify_proc;

    uint32_t m_current_user_id = 0;
    std::vector<GeoRow> m_current_rows;
    std::vector<GeoSample> m_current_samples;

    void flush_user_rows();

public:
    SearchProcess(const Config* cfg, const Sampler* sampler,
        const GeoSearch* search, const SickMap* sick_map, NotifyProcess* notify_proc);
    void process_query_row(const GeoRow& row);
    void close();
};

}
