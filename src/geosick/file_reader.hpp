#pragma once
#include <cstdio>
#include <filesystem>
#include <string>
#include "geosick/geo_row_reader.hpp"

namespace geosick {

class FileReader final: public GeoRowReader {
    FILE* m_file = nullptr;
public:
    explicit FileReader(const std::filesystem::path& path) {
        m_file = std::fopen(path.c_str(), "r");
        if (!m_file) {
            throw std::runtime_error(
                "Could not open file for reading: " + path.string());
        }
    }
    ~FileReader() { this->close(); }

    virtual std::optional<GeoRow> read() override {
        if (!m_file) { return {}; }
        GeoRow row;
        if (std::fread(&row, sizeof(row), 1, m_file) != 1) {
            if (std::feof(m_file)) { return {}; }
            throw std::runtime_error("Error when reading GeoRow from file");
        }
        return row;
    }

    void close() {
        if (m_file) {
            std::fclose(m_file);
            m_file = nullptr;
        }
    }
};

}
