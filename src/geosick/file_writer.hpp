#pragma once
#include <cstdio>
#include <filesystem>
#include <string>
#include "geosick/geo_row.hpp"
#include "geosick/slice.hpp"

namespace geosick {

class FileWriter final {
    FILE* m_file = nullptr;
public:
    explicit FileWriter(const std::filesystem::path& path) {
        m_file = std::fopen(path.c_str(), "w");
        if (!m_file) {
            throw std::runtime_error(
                "Could not open file for writing: " + path.string());
        }
    }
    ~FileWriter() { this->close(); }

    void write(const GeoRow& row) {
        this->write(make_view(&row, &row + 1));
    }

    void write(ArrayView<GeoRow> rows) {
        if (!m_file) {
            throw std::runtime_error("Cannot write to a closed file");
        }
        size_t res = std::fwrite(rows.begin(), sizeof(GeoRow), rows.size(), m_file);
        if (res != rows.size()) {
            throw std::runtime_error("Error when writing GeoRow-s to file");
        }
    }

    void close() {
        if (m_file) {
            std::fclose(m_file);
            m_file = nullptr;
        }
    }

};

}
