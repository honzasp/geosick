#pragma once
#include <fstream>
#include <filesystem>
#include <rapidjson/stringbuffer.h>
#include "geosick/match.hpp"

namespace geosick {

class Sampler;

class NotifyProcess {
    const Sampler* m_sampler;
    std::ofstream m_matches_output;
    rapidjson::StringBuffer m_match_buffer;
public:
    NotifyProcess(const Sampler* sampler, const std::filesystem::path& matches_path);
    void notify(const MatchInput& mi, const MatchOutput& mo);
    void close();
};

}
