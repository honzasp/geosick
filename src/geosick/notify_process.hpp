#pragma once
#include <bzlib.h>
#include <fstream>
#include <filesystem>
#include <rapidjson/stringbuffer.h>
#include <random>
#include "geosick/match.hpp"

namespace geosick {

class Sampler;

class NotifyProcess {
    const Config* m_cfg;
    const Sampler* m_sampler;
    uint64_t m_match_count { 0 };

    std::ofstream m_json_output;
    FILE* m_selected_json_file { nullptr };
    BZFILE* m_selected_json_bzfile { nullptr };
    std::mt19937 m_rng;
    rapidjson::StringBuffer m_json_buffer;
    uint64_t m_json_count { 0 };
    uint64_t m_selected_json_count { 0 };

    void notify_json(const MatchInput& mi, const MatchOutput& mo);
public:
    NotifyProcess(const Config* cfg, const Sampler* sampler,
      const std::filesystem::path& matches_path,
      const std::filesystem::path& selected_matches_path);
    ~NotifyProcess();
    void notify(const MatchInput& mi, const MatchOutput& mo);
    void close();
};

}
