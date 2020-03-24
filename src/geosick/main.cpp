#include <iostream>
#include "geosick/file_writer.hpp"
#include "geosick/mysql_reader.hpp"
#include "geosick/preprocess.hpp"

namespace geosick {

static void print_row(const GeoRow& row) {
    std::cout << "Row"
        << ": user " << row.user_id
        << ", timestamp " << row.timestamp_utc_s
        << ", lat " << row.lat
        << ", lon " << row.lon
        << ", accuracy " << row.accuracy_m
        << ", altitude " << row.altitude_m
        << ", heading " << row.heading_deg
        << ", speed " << row.speed_mps
        << std::endl;
}

static void main() {
    std::filesystem::path temp_dir = "/tmp/geosick";
    std::unordered_set<uint32_t> infected_user_ids {5, 7};
    Preprocess preprocess(infected_user_ids, temp_dir, 10); {
        MysqlReader mysql("zostanzdravy_app", "127.0.0.1", 3306, "root", "");
        preprocess.process(mysql);
    }

    auto all_reader = preprocess.read_all_rows();
    FileWriter all_writer(temp_dir / "all_rows.bin");
    while (auto row = all_reader->read()) {
        all_writer.write(*row);
        print_row(*row);
    }
    all_writer.close();
}

}

int main() {
    try {
        geosick::main();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
