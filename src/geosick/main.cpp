#include "geosick/file_reader.hpp"
#include "geosick/file_writer.hpp"
#include "geosick/merge_reader.hpp"
#include "geosick/mysql_reader.hpp"

namespace geosick {

static void print_row(const GeoRow& row) {
    std::cout << "Row"
        << ": user " << row.user_id
        << ", timestamp " << row.timestamp_utc_s
        << ", lat " << row.lat
        << ", lon " << row.lon
        << ", accuracy " << row.accuracy_m
        << ", altitude " << row.altitude_m
        << ", heading " << row.heading_deg
        << ", speed " << row.speed_mps
        << std::endl;
}

void main() {
    const auto compare_rows = [](const GeoRow& r1, const GeoRow& r2) {
        if (r1.user_id != r2.user_id) { return r1.user_id < r2.user_id; }
        return r1.timestamp_utc_s < r2.timestamp_utc_s;
    };

    std::unordered_map<uint32_t, uint32_t> infected_timestamps {
        {5, 1584732600},
        {7, 1584734064},
    };

    std::filesystem::path temp_dir = "/tmp/geosick";
    std::vector<std::filesystem::path> temp_paths;
    uint32_t temp_file_counter = 0;
    std::vector<GeoRow> buffered_rows;
    std::unordered_map<uint32_t, std::vector<GeoRow>> infected_rows;

    auto flush_buffered = [&] {
        std::sort(buffered_rows.begin(), buffered_rows.end(), compare_rows);

        auto temp_name = "rows_" + std::to_string(temp_file_counter++) + ".bin";
        auto temp_path = temp_dir / temp_name;
        temp_paths.push_back(temp_path);

        FileWriter writer(temp_path);
        for (const auto& row: buffered_rows) {
            writer.write(row);
        }
        buffered_rows.clear();
    };

    MysqlReader mysql("zostanzdravy_app", "127.0.0.1", 3306, "root", "");
    while (auto row = mysql.read()) {
        auto timestamp_iter = infected_timestamps.find(row->user_id);
        if (timestamp_iter != infected_timestamps.end()) {
            uint32_t infected_timestamp = timestamp_iter->second;
            if (row->timestamp_utc_s < infected_timestamp) { continue; }

            auto rows_iter = infected_rows.find(row->user_id);
            if (rows_iter == infected_rows.end()) {
                rows_iter = infected_rows.emplace(
                    row->user_id, std::vector<GeoRow>()).first;
            }
            rows_iter->second.push_back(*row);
            continue;
        }
                    
        buffered_rows.push_back(*row);
        if (buffered_rows.size() >= 1000) {
            flush_buffered();
        }
    }
    if (!buffered_rows.empty()) {
        flush_buffered();
    }

    MergeReader<decltype(compare_rows)> merge(compare_rows);
    for (const auto& temp_path: temp_paths) {
        merge.add_reader(std::make_unique<FileReader>(temp_path));
        std::filesystem::remove(temp_path);
    }
    FileWriter writer(temp_dir / "all_rows.bin");
    while (auto row = merge.read()) {
        print_row(*row);
        writer.write(*row);
    }
    writer.close();
}

}

int main() {
    try {
        geosick::main();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
