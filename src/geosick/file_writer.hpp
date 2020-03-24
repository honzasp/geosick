#pragma once
#include <cstdio>
#include <string>

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
        if (!m_file) {
            throw std::runtime_error("Cannot write to a closed file");
        }
        if (std::fwrite(&row, sizeof(row), 1, m_file) != 1) {
            throw std::runtime_error("Error when writing GeoRow to file");
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
