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
    cfg.mysql.db = doc["mysql"]["db"].get<std::string>();
    cfg.mysql.host = doc["mysql"]["host"].get<std::string>();
    cfg.mysql.port = doc["mysql"]["port"].get<uint32_t>();
    cfg.mysql.user = doc["mysql"]["user"].get<std::string>();
    cfg.mysql.password = doc["mysql"]["password"].get<std::string>();
    cfg.range_days = doc["range_days"].get<uint32_t>();
    cfg.period_s = doc["period_s"].get<uint32_t>();
    cfg.temp_dir = doc["temp_dir"].get<std::string>();
    cfg.row_buffer_size = doc["row_buffer_size"].get<uint32_t>();
    return cfg;
}

static nlohmann::json row_to_json(const GeoRow& row) {
    nlohmann::json doc = {
        {"timestamp_ms", int64_t(row.timestamp_utc_s) * 1000},
        {"latitude_e7", row.lat},
        {"longitude_e7", row.lon},
        {"accuracy_m", row.accuracy_m},
    };
    if (row.altitude_m != UINT16_MAX) {
        doc["altitude_m"] = row.altitude_m;
    }
    if (row.heading_deg != UINT16_MAX) {
        doc["heading_deg"] = row.heading_deg;
    }
    doc["velocity_mps"] = row.velocity_mps;
    return doc;
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

static nlohmann::json request_to_json(
    const std::vector<GeoRow>& sick_rows,
    const std::vector<GeoRow>& query_rows)
{
    nlohmann::json doc;
    doc["sick_geopoints"] = nlohmann::json::array();
    for (const auto& row: sick_rows) {
        doc["sick_geopoints"].push_back(row_to_json(row));
    }
    doc["query_geopoints"] = nlohmann::json::array();
    for (const auto& row: query_rows) {
        doc["query_geopoints"].push_back(row_to_json(row));
    }
    return doc;
}

static void main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <config-file> <output-file>" << std::endl;
        throw std::runtime_error("Bad usage");
    }

    nlohmann::json config_doc; {
        std::ifstream config_file(argv[1]);
        config_file >> config_doc;
    }
    Config cfg = config_from_json(config_doc);
    std::filesystem::path temp_dir = cfg.temp_dir;
    MysqlDb mysql(cfg);

    auto sick_user_ids = mysql.read_sick_user_ids();
    ReadProcess read_proc(sick_user_ids, temp_dir, cfg.row_buffer_size); {
        auto row_reader = mysql.read_rows();
        read_proc.process(*row_reader);
    }

    auto end_time = UtcTime(DurationS(read_proc.get_max_timestamp()));
    auto begin_time = end_time - DurationS(24*60*60 * cfg.range_days);
    auto period = DurationS(cfg.period_s);
    Sampler sampler(begin_time, end_time, period);

    auto sick_samples = rows_to_samples(sampler, read_proc.read_sick_rows());
    GeoSearch search(sick_samples);

    FileWriter all_writer(temp_dir / "all_rows.bin");
    SearchProcess search_proc(&sampler, &search, &all_writer, &sick_user_ids);
    auto proc_reader = read_proc.read_all_rows();
    while (auto row = proc_reader->read()) {
        search_proc.process_row(*row);
    }
    search_proc.process_end();

    auto hits = search_proc.read_hits(); {
        std::ofstream output_file(argv[2]);
        for (auto hit: hits) {
            auto query_rows = search_proc.read_user_rows(hit.query_user_id);
            auto sick_rows = search_proc.read_user_rows(hit.sick_user_id);
            output_file << request_to_json(sick_rows, query_rows) << std::endl;
        }
    };
    all_writer.close();
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
