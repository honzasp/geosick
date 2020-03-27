#include <nlohmann/json.hpp>
#include "geosick/notify_process.hpp"
#include "geosick/sampler.hpp"

namespace geosick {

NotifyProcess::NotifyProcess(const Sampler* sampler,
    const std::filesystem::path& matches_path)
{
    m_sampler = sampler;
    m_matches_output.open(matches_path);
}

static nlohmann::json row_to_json(const GeoRow& row) {
    nlohmann::json doc {
        {"user_id", row.user_id},
        {"timestamp_utc_s", row.timestamp_utc_s},
        {"lat_e7", row.lat},
        {"lon_e7", row.lon},
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

static nlohmann::json timestamp_to_json(UtcTime time) {
    return time.time_since_epoch().count();
}

static nlohmann::json sample_to_json(const Sampler& sampler, const GeoSample& sample) {
    nlohmann::json doc;
    doc["time_index"] = sample.time_index;
    doc["timestamp_utc_s"] = timestamp_to_json(
        sampler.time_index_to_timestamp(sample.time_index));
    doc["user_id"] = sample.user_id;
    doc["lat_e7"] = sample.lat;
    doc["lon_e7"] = sample.lon;
    doc["accuracy_m"] = (double)sample.accuracy_m;
    return doc;
}

static nlohmann::json step_to_json(const Sampler& sampler, const MatchStep& step) {
    nlohmann::json doc;
    doc["time_index"] = step.time_index;
    doc["timestamp_utc_s"] = timestamp_to_json(
        sampler.time_index_to_timestamp(step.time_index));
    doc["infect_rate"] = step.infect_rate;
    doc["distance_m"] = step.distance_m;
    return doc;
}

static nlohmann::json match_to_json(const Sampler& sampler,
    const MatchInput& mi, const MatchOutput& mo)
{
    nlohmann::json doc;
    doc["query_user_id"] = mi.query_user_id;
    doc["sick_user_id"] = mi.sick_user_id;

    doc["query_rows"] = nlohmann::json::array();
    doc["sick_rows"] = nlohmann::json::array();
    for (const auto& query_row: mi.query_rows) {
        doc["query_rows"].push_back(row_to_json(query_row));
    }
    for (const auto& sick_row: mi.sick_rows) {
        doc["sick_rows"].push_back(row_to_json(sick_row));
    }

    doc["query_samples"] = nlohmann::json::array();
    doc["sick_samples"] = nlohmann::json::array();
    for (const auto& query_sample: mi.query_samples) {
        doc["query_samples"].push_back(sample_to_json(sampler, query_sample));
    }
    for (const auto& sick_sample: mi.sick_samples) {
        doc["sick_samples"].push_back(sample_to_json(sampler, sick_sample));
    }

    doc["steps"] = nlohmann::json::array();
    for (const auto& step: mo.steps) {
        doc["steps"].push_back(step_to_json(sampler, step));
    }

    doc["score"] = mo.score;
    doc["min_distance_m"] = mo.min_distance_m;
    return doc;
}

void NotifyProcess::notify(const MatchInput& mi, const MatchOutput& mo) {
    m_matches_output << match_to_json(*m_sampler, mi, mo) << std::endl;
}

void NotifyProcess::close() {
    m_matches_output.close();
}

}
