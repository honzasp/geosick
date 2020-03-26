#pragma once
#include <filesystem>
#include <vector>
#include <unordered_set>
#include "geosick/geo_row_reader.hpp"

namespace geosick {

class ReadProcess {
    const std::unordered_set<uint32_t>* m_sick_user_ids;
    const std::unordered_set<uint32_t>* m_query_user_ids;
    std::filesystem::path m_temp_dir;
    size_t m_row_buffer_size;

    std::vector<GeoRow> m_row_buffer;
    std::vector<GeoRow> m_sick_rows;
    std::vector<std::vector<std::filesystem::path>> m_temp_files;
    uint32_t m_temp_file_counter = 0;
    uint32_t m_min_timestamp = UINT32_MAX;
    uint32_t m_max_timestamp = 0;

    void flush_buffer();
    void add_temp_file(std::filesystem::path path, size_t level);
    std::filesystem::path merge_temp_files(
        const std::vector<std::filesystem::path>& files);
    std::filesystem::path gen_temp_file();
public:
    ReadProcess(const std::unordered_set<uint32_t>* sick_user_ids,
        const std::unordered_set<uint32_t>* query_user_ids,
        std::filesystem::path temp_dir,
        size_t row_buffer_size);
    void process(GeoRowReader& reader);

    std::unique_ptr<GeoRowReader> read_query_rows();
    std::vector<GeoRow> read_sick_rows();

    uint32_t get_min_timestamp() const { return m_min_timestamp; }
    uint32_t get_max_timestamp() const { return m_max_timestamp; }
};

}
