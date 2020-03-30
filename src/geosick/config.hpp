#pragma once
#include <string>

namespace geosick {

struct Config {
    struct Mysql {
        std::string db;
        std::string server;
        std::string user;
        std::string password;
    } mysql;

    struct Search {
        uint32_t bucket_count;
        double bin_delta_m;
    } search;

    uint32_t range_days;
    uint32_t period_s;
    std::string temp_dir;
    uint32_t row_buffer_size;
};

}
