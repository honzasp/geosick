#include <iostream>
#include "geosick/file_writer.hpp"
#include "geosick/geo_search.hpp"
#include "geosick/mysql_reader.hpp"
#include "geosick/read_process.hpp"
#include "geosick/sampler.hpp"
#include "geosick/search_process.hpp"

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

static std::vector<GeoSample> rows_to_samples(
    const Sampler& sampler, const std::vector<GeoRow>& rows)
{
    std::vector<GeoSample> samples;
    size_t user_begin = 0;
    while (user_begin < rows.size()) {
        uint32_t user_id = rows.at(user_begin).user_id;
        size_t user_end = user_begin + 1;
        while (user_end < rows.size() && rows.at(user_end).user_id == user_id) {
            ++user_end;
        }

        auto user_view = make_view(rows.data() + user_begin, rows.data() + user_end);
        sampler.sample(user_view, samples);
        user_begin = user_end;
    }
    return samples;
}

static void main() {
    std::filesystem::path temp_dir = "/tmp/geosick";
    std::unordered_set<uint32_t> infected_user_ids {5, 7};
    ReadProcess read_proc(infected_user_ids, temp_dir, 10); {
        MysqlReader mysql("zostanzdravy_app", "127.0.0.1", 3306, "root", "");
        read_proc.process(mysql);
    }

    auto end_time = UtcTime(DurationS(read_proc.get_max_timestamp()));
    auto begin_time = end_time - DurationS(60*60);
    auto period = DurationS(60);
    Sampler sampler(begin_time, end_time, period);

    auto infected_samples = rows_to_samples(sampler, read_proc.read_infected_rows());
    GeoSearch search(infected_samples);

    FileWriter all_writer(temp_dir / "all_rows.bin");
    SearchProcess search_proc(&sampler, &search, &all_writer);
    auto proc_reader = read_proc.read_all_rows();
    while (auto row = proc_reader->read()) {
        search_proc.process_row(*row);
    }
    search_proc.process_end();

    auto hits = search_proc.read_hits();
    for (auto hit: hits) {
        std::cout << "HIT " << hit.healthy_user_id << " vs " 
            << hit.infected_user_id << std::endl;
        std::cout << "Healthy rows:" << std::endl;
        for (auto row: search_proc.read_user_rows(hit.healthy_user_id)) {
            print_row(row);
        }
        std::cout << "Infected rows:" << std::endl;
        for (auto row: search_proc.read_user_rows(hit.infected_user_id)) {
            print_row(row);
        }
        std::cout << std::endl;
    }

    all_writer.close();
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
