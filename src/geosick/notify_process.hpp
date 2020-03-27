#pragma once
#include <fstream>
#include <filesystem>
#include "geosick/match.hpp"

namespace geosick {

class Sampler;

class NotifyProcess {
    const Sampler* m_sampler;
    std::ofstream m_matches_output;
public:
    NotifyProcess(const Sampler* sampler, const std::filesystem::path& matches_path);
    void notify(const MatchInput& mi, const MatchOutput& mo);
    void close();
};

}
