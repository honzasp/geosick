#pragma once
#include <string>

namespace geosick {

struct Config {
    struct Mysql {
        std::string db;
        std::string host;
        uint32_t port;
        std::string user;
        std::string password;
    } mysql;

    uint32_t range_days;
    uint32_t period_s;
    std::string temp_dir;
    uint32_t row_buffer_size;
};

}
