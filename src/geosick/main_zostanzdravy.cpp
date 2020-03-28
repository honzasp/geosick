#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include "geosick/file_writer.hpp"
#include "geosick/geo_search.hpp"
#include "geosick/mysql_db.hpp"
#include "geosick/read_process.hpp"
#include "geosick/sampler.hpp"
#include "geosick/search_process.hpp"

namespace geosick {

static Config config_from_json(const rapidjson::Value& doc) {
    Config cfg;
    cfg.mysql.db = doc["mysql"]["db"].GetString();
    cfg.mysql.host = doc["mysql"]["host"].GetString();
    cfg.mysql.port = doc["mysql"]["port"].GetUint();
    cfg.mysql.user = doc["mysql"]["user"].GetString();
    cfg.mysql.password = doc["mysql"]["password"].GetString();
    cfg.range_days = doc["range_days"].GetUint();
    cfg.period_s = doc["period_s"].GetUint();
    cfg.temp_dir = doc["temp_dir"].GetString();
    cfg.row_buffer_size = doc["row_buffer_size"].GetUint();
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
    rapidjson::Document config_doc; {
        std::ifstream config_file(argv[1]);
        rapidjson::BasicIStreamWrapper config_stream(config_file);
        config_doc.ParseStream(config_stream);
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

    auto end_time = UtcTime(DurationS(read_proc.get_max_timestamp()));
    auto begin_time = end_time - DurationS(24*60*60 * cfg.range_days);
    auto period = DurationS(cfg.period_s);
    Sampler sampler(begin_time, end_time, period);

    std::cout << "Building the search structure..." << std::endl;
    auto sick_map = read_sick_map(sampler, read_proc.read_sick_rows());
    GeoSearch search(sick_map.samples);

    std::cout << "Searching for matches..." << std::endl;
    NotifyProcess notify_proc(&sampler, temp_dir / "matches.json");
    SearchProcess search_proc(&cfg, &sampler, &search, &sick_map, &notify_proc);
    auto reader = read_proc.read_query_rows();
    while (auto row = reader->read()) {
        search_proc.process_query_row(*row);
    }
    search_proc.close();
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
