#pragma once
#include <cstdio>
#include <filesystem>
#include <string>
#include "geosick/geo_row_reader.hpp"

namespace geosick {

class FileReader final: public GeoRowReader {
    FILE* _file = nullptr;
public:
    explicit FileReader(const std::filesystem::path& path) {
        this->_file = std::fopen(path.c_str(), "r");
        if (!this->_file) {
            throw std::runtime_error(
                "Could not open file for reading: " + path.string());
        }
    }
    ~FileReader() { this->close(); }

    virtual std::optional<GeoRow> read() override {
        if (!this->_file) { return {}; }
        GeoRow row;
        if (std::fread(&row, sizeof(row), 1, this->_file) != 1) {
            if (std::feof(this->_file)) { return {}; }
            throw std::runtime_error("Error when reading GeoRow from file");
        }
        return row;
    }

    void close() {
        if (this->_file) {
            std::fclose(this->_file);
            this->_file = nullptr;
        }
    }
};

}
