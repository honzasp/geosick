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
    const Sampler* m_sampler;
    std::ofstream m_matches_output;
    FILE* m_selected_matches_file { nullptr };
    BZFILE* m_selected_matches_bzfile { nullptr };
    std::mt19937 m_rng;
    rapidjson::StringBuffer m_match_buffer;
    uint64_t m_match_count { 0 };
public:
    NotifyProcess(const Sampler* sampler,
      const std::filesystem::path& matches_path,
      const std::filesystem::path& selected_matches_path);
    ~NotifyProcess();
    void notify(const MatchInput& mi, const MatchOutput& mo);
    void close();
};

}
