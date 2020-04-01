#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "geosick/file_writer.hpp"
#include "geosick/geo_search.hpp"
#include "geosick/mysql_db.hpp"
#include "geosick/read_process.hpp"
#include "geosick/sampler.hpp"
#include "geosick/search_process.hpp"

namespace geosick {

static Config config_from_json(const nlohmann::json& doc) {
    Config cfg;
    auto mysql_doc = doc.at("mysql");
    cfg.mysql.db = mysql_doc.at("db").get<std::string>();
    cfg.mysql.server = mysql_doc.at("server").get<std::string>();
    cfg.mysql.user = mysql_doc.at("user").get<std::string>();
    cfg.mysql.password = mysql_doc.at("password").get<std::string>();
    cfg.mysql.ssl_mode = mysql_doc.value<std::string>("ssl_mode", "PREFERRED");

    auto search_doc = doc["search"];
    cfg.search.bucket_count = search_doc.value<uint32_t>("bucket_count", 1000);
    cfg.search.bin_delta_m = search_doc.value<double>("bin_delta_m", 200.0);

    cfg.range_days = doc.value<uint32_t>("range_days", 14);
    cfg.period_s = doc.value<uint32_t>("period_s", 30);
    cfg.temp_dir = doc.at("temp_dir").get<std::string>();
    cfg.row_buffer_size = doc.value<uint32_t>("row_buffer_size", 40000000);
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

    std::cout << "Initializing..." << std::endl;
    nlohmann::json config_doc; {
        std::ifstream config_file(argv[1]);
        config_file >> config_doc;
    }
    Config cfg = config_from_json(config_doc);
    std::filesystem::path temp_dir = cfg.temp_dir;
    MysqlDb mysql(cfg);

    auto user_ids = mysql.read_user_ids();
    std::cout << "Reading rows..." << std::endl;
    ReadProcess read_proc(&user_ids.sick, &user_ids.query, temp_dir, cfg.row_buffer_size);
    {
        auto row_reader = mysql.read_rows();
        read_proc.process(*row_reader);
    }

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
    auto sick_map = read_sick_map(sampler, read_proc.read_sick_rows());
    GeoSearch search(cfg, make_view(sick_map.samples));

    std::cout << "Searching for matches..." << std::endl;
    NotifyProcess notify_proc(&sampler,
        temp_dir / "matches.json", temp_dir / "selected_matches.json.bz2");
    SearchProcess search_proc(&cfg, &sampler, &search, &sick_map, &notify_proc);
    auto reader = read_proc.read_query_rows();
    while (auto row = reader->read()) {
        search_proc.process_query_row(*row);
    }
    search_proc.close();
    search.close();
    notify_proc.close();

    std::cout << "Done" << std::endl;
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
