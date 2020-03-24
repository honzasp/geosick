#pragma once
#include <cstdio>
#include <filesystem>
#include <string>
#include <unistd.h>
#include "geosick/geo_row.hpp"
#include "geosick/slice.hpp"

namespace geosick {

class FileWriter final {
    FILE* m_file = nullptr;
    size_t m_offset = 0;
public:
    explicit FileWriter(const std::filesystem::path& path) {
        m_file = std::fopen(path.c_str(), "w+");
        if (!m_file) {
            throw std::runtime_error(
                "Could not open file for writing: " + path.string());
        }
    }
    ~FileWriter() { this->close(); }

    void write(const GeoRow& row) {
        this->write(make_view(&row, &row + 1));
    }

    void write(ArrayView<const GeoRow> rows) {
        if (!m_file) {
            throw std::runtime_error("Cannot write to a closed file");
        }
        size_t res = std::fwrite(rows.begin(), sizeof(GeoRow), rows.size(), m_file);
        if (res != rows.size()) {
            throw std::runtime_error("Error when writing GeoRow-s to file");
        }
        m_offset += rows.size() * sizeof(GeoRow);
    }

    size_t get_offset() const { return m_offset; }

    void flush() {
        if (m_file) { std::fflush(m_file); }
    }

    void close() {
        if (m_file) {
            std::fclose(m_file);
            m_file = nullptr;
        }
    }

    void pread(size_t offset, ArrayView<GeoRow> rows) {
        ssize_t res = ::pread(::fileno(m_file), rows.begin(),
            sizeof(GeoRow) * rows.size(), offset);
        if (res < 0 || (size_t)res != sizeof(GeoRow) * rows.size()) {
            throw std::runtime_error("Error when preading GeoRow-s from file");
        }
    }


};

}
