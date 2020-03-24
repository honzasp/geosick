#pragma once
#include <cstdio>
#include <string>

namespace geosick {

class FileWriter final {
    FILE* _file = nullptr;
public:
    explicit FileWriter(const std::filesystem::path& path) {
        this->_file = std::fopen(path.c_str(), "w");
        if (!this->_file) {
            throw std::runtime_error(
                "Could not open file for writing: " + path.string());
        }
    }
    ~FileWriter() { this->close(); }

    void write(const GeoRow& row) {
        if (!this->_file) {
            throw std::runtime_error("Cannot write to a closed file");
        }
        if (std::fwrite(&row, sizeof(row), 1, this->_file) != 1) {
            throw std::runtime_error("Error when writing GeoRow to file");
        }
    }

    void close() {
        if (this->_file) {
            std::fclose(this->_file);
            this->_file = nullptr;
        }
    }

};

}
