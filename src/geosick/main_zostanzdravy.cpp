#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "geosick/file_writer.hpp"
#include "geosick/geo_distance.hpp"
#include "geosick/geo_search.hpp"
#include "geosick/mysql_db.hpp"
#include "geosick/read_process.hpp"
#include "geosick/sampler.hpp"
#include "geosick/search_process.hpp"

namespace geosick {

namespace {
    struct Stopwatch {
        using Clock = std::chrono::high_resolution_clock;
        Clock::time_point m_start_time;

        Stopwatch() {
            m_start_time = Clock::now();
        }
        double get_s() const {
            return std::chrono::duration<double>(Clock::now() - m_start_time).count();
        }
    };
}

static Config config_from_json(nlohmann::json& doc) {
    auto p = [&](const char* ptr) { return nlohmann::json::json_pointer(ptr); };

    Config cfg;
    cfg.mysql.db = doc.at(p("/mysql/db")).get<std::string>();
    cfg.mysql.server = doc.at(p("/mysql/server")).get<std::string>();
    cfg.mysql.user = doc.at(p("/mysql/user")).get<std::string>();
    cfg.mysql.password = doc.at(p("/mysql/password")).get<std::string>();
    cfg.mysql.ssl_mode = doc.value<std::string>(p("/mysql/ssl_mode"), "PREFERRED");

    cfg.search.bucket_count = doc.value<uint32_t>(p("/search/bucket_count"), 1000);
    cfg.search.bin_delta_m = doc.value<double>(p("/search/bin_delta_m"), 200.0);

    cfg.notify.use_json = doc.value<bool>(p("/notify/use_json"), true);
    cfg.notify.json_min_score = doc.value<double>(p("/notify/json_min_score"), 0.001);
    cfg.notify.use_db = doc.value<bool>(p("/notify/use_db"), false);
    cfg.notify.db_min_score = doc.value<double>(p("/notify/db_min_score"), 0.1);

    cfg.range_days = doc.value<uint32_t>(p("/range_days"), 14);
    cfg.period_s = doc.value<uint32_t>(p("/period_s"), 30);
    cfg.temp_dir = doc.at(p("/temp_dir")).get<std::string>();
    cfg.row_buffer_size = doc.value<uint32_t>(p("/row_buffer_size"), 40000000);
    return cfg;
}

static SickMap read_sick_map(const Sampler& sampler, std::vector<GeoRow> sick_rows) {
    SickMap map;
    map.rows = std::move(sick_rows);

    size_t user_idx = 0;
    size_t user_begin = 0;
    while (user_begin < map.rows.size()) {
        uint32_t user_id = map.rows.at(user_begin).user_id;
        size_t user_end = user_begin + 1;
        while (user_end < map.rows.size() && map.rows.at(user_end).user_id == user_id) {
            ++user_end;
        }

        map.user_id_to_idx.emplace(user_id, user_idx);
        map.row_offsets.push_back(user_begin);
        map.sample_offsets.push_back(map.samples.size());
        ArrayView<const GeoRow> rows_view {
            map.rows.data() + user_begin, map.rows.data() + user_end};
        sampler.sample(rows_view, map.samples);

        user_idx += 1;
        user_begin = user_end;
    }

    map.row_offsets.push_back(user_begin);
    map.sample_offsets.push_back(map.samples.size());
    return map;
}

static void main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config-file>" << std::endl;
        throw std::runtime_error("Bad usage");
    }
    Stopwatch all_sw;

    std::cout << "Initializing..." << std::endl;
    nlohmann::json config_doc; {
        std::ifstream config_file(argv[1]);
        config_file >> config_doc;
    }
    Config cfg = config_from_json(config_doc);
    std::filesystem::path temp_dir = cfg.temp_dir;
    MysqlDb mysql(cfg);

    std::cout << "Reading rows..." << std::endl;
    Stopwatch read_sw;
    auto user_ids = mysql.read_user_ids();
    ReadProcess read_proc(&user_ids.sick, &user_ids.query, temp_dir, cfg.row_buffer_size);
    {
        auto row_reader = mysql.read_rows();
        read_proc.process(*row_reader);
    }
    std::cout << "  reading took " << read_sw.get_s() << " s" << std::endl;

    int32_t mysql_time = mysql.read_now_timestamp();
    int32_t end_time = mysql_time;
    int32_t begin_time = end_time - 24*60*60 * (int32_t)cfg.range_days;
    int32_t period = (int32_t)cfg.period_s;
    std::cout << "Sampling:" << std::endl
        << "  mysql timestamp: " << mysql_time << std::endl
        << "  begin timestamp: " << begin_time << std::endl
        << "  end timestamp: " << end_time << std::endl
        << "  period: " << period << std::endl;
    Sampler sampler(begin_time, end_time, period);

    std::cout << "Building the search structure..." << std::endl;
    Stopwatch build_sw;
    auto sick_map = read_sick_map(sampler, read_proc.read_sick_rows());
    GeoSearch search(cfg, make_view(sick_map.samples));
    std::cout << "  building took " << build_sw.get_s() << " s" << std::endl;

    std::cout << "Searching for matches..." << std::endl;
    Stopwatch search_sw;
    NotifyProcess notify_proc(&cfg, &sampler,
        temp_dir / "matches.json", temp_dir / "selected_matches.json.bz2");
    SearchProcess search_proc(&cfg, &sampler, &search, &sick_map, &notify_proc);
    auto reader = read_proc.read_query_rows();
    while (auto row = reader->read()) {
        search_proc.process_query_row(*row);
    }
    std::cout << "  searching took " << search_sw.get_s() << " s" << std::endl;
    search_proc.close();
    search.close();
    notify_proc.close();

    std::cout << "Done in " << all_sw.get_s() << " s" << std::endl;
}

}

int main(int argc, char** argv) {
    try {
        geosick::main(argc, argv);
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
