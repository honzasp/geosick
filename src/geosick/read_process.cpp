#include <algorithm>
#include <future>
#include <iostream>
#include "geosick/file_reader.hpp"
#include "geosick/file_writer.hpp"
#include "geosick/merge_reader.hpp"
#include "geosick/read_process.hpp"

namespace geosick {

namespace {
    struct CompareRows {
        bool operator()(const GeoRow& r1, const GeoRow& r2) {
            if (r1.user_id < r2.user_id) { return true; }
            if (r1.user_id > r2.user_id) { return false; }
            return r1.timestamp_utc_s < r2.timestamp_utc_s;
        }
    };
}


ReadProcess::ReadProcess(const std::unordered_set<uint32_t>* sick_user_ids,
    const std::unordered_set<uint32_t>* query_user_ids,
    std::filesystem::path temp_dir,
    size_t row_buffer_size
):
    m_sick_user_ids(sick_user_ids),
    m_query_user_ids(query_user_ids),
    m_temp_dir(std::move(temp_dir)),
    m_row_buffer_size(row_buffer_size)
{}

void ReadProcess::flush_buffer(std::vector<GeoRow> buffer) {
    std::sort(buffer.begin(), buffer.end(), CompareRows());

    std::unique_lock<std::mutex> lock(m_mutex);
    auto temp_path = this->gen_temp_file();
    lock.unlock();

    FileWriter writer(temp_path);
    writer.write(make_view(buffer));
    writer.close();

    lock.lock();
    this->add_temp_file(lock, temp_path, 0);
}

static void merge_temp_files(const std::filesystem::path& out_file,
    const std::vector<std::filesystem::path>& files)
{
    MergeReader<CompareRows> merger { CompareRows() };
    for (const auto& path: files) {
        merger.add_reader(std::make_unique<FileReader>(path));
        std::filesystem::remove(path);
    }

    FileWriter writer(out_file);
    while (auto row = merger.read()) {
        writer.write(*row);
    }
}

void ReadProcess::add_temp_file(std::unique_lock<std::mutex>& lock,
    std::filesystem::path path, size_t level)
{
    const size_t MAX_MERGE_SIZE = 4;
    for (;;) {
        while (m_temp_files.size() <= level) {
            m_temp_files.emplace_back();
        }
        m_temp_files.at(level).push_back(path);
        if (m_temp_files.at(level).size() <= MAX_MERGE_SIZE) { break; }

        path = this->gen_temp_file();
        lock.unlock();
        merge_temp_files(path, m_temp_files.at(level));
        lock.lock();
        m_temp_files.at(level).clear();
        level += 1;
    }
}

std::filesystem::path ReadProcess::gen_temp_file() {
    auto temp_name = "rows_" + std::to_string(m_temp_file_counter++) + ".bin";
    return m_temp_dir / temp_name;
}

void ReadProcess::process(GeoRowReader& reader) {
    std::future<void> flush_future;
    std::vector<GeoRow> buffer;
    auto flush = [&] {
        std::cout << "  Flush " << buffer.size() << " rows" << std::endl;
        m_query_row_count += buffer.size();
        if (flush_future.valid()) { flush_future.get(); }
        flush_future = std::async(std::launch::async,
            &ReadProcess::flush_buffer, this, std::move(buffer));
        buffer.clear();
    };

    buffer.reserve(m_row_buffer_size);
    while (auto row = reader.read()) {
        m_min_timestamp = std::min(m_min_timestamp, row->timestamp_utc_s);
        m_max_timestamp = std::max(m_max_timestamp, row->timestamp_utc_s);

        if (m_sick_user_ids->count(row->user_id)) {
            m_sick_rows.push_back(*row);
        } else if (m_query_user_ids->count(row->user_id)) {
            buffer.push_back(*row);
            if (buffer.size() >= m_row_buffer_size) {
                flush();
                buffer.reserve(m_row_buffer_size);
            }
        }
    }

    if (buffer.size() > 0) {
        flush();
    }
    std::sort(m_sick_rows.begin(), m_sick_rows.end(), CompareRows());
    if (flush_future.valid()) { flush_future.get(); }

    std::cout << "  Loaded " << m_query_row_count << " query rows, "
        << m_sick_rows.size() << " sick rows" << std::endl;
}

std::unique_ptr<GeoRowReader> ReadProcess::read_query_rows() {
    auto merger = std::make_unique<MergeReader<CompareRows>>(CompareRows());
    for (auto& paths: m_temp_files) {
        for (const auto& path: paths) {
            merger->add_reader(std::make_unique<FileReader>(path));
            std::filesystem::remove(path);
        }
        paths.clear();
    }
    return merger;
}

std::vector<GeoRow> ReadProcess::read_sick_rows() {
    return std::move(m_sick_rows);
}

}
