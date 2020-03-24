#pragma once
#include <algorithm>
#include <vector>
#include "geosick/geo_row_reader.hpp"

namespace geosick {

template<class Compare>
class MergeReader final: public GeoRowReader {
    Compare _compare;
    
    struct HeapEntry {
        GeoRow row;
        std::unique_ptr<GeoRowReader> reader;
    };
    std::vector<HeapEntry> _heap;

    void advance() {
        if (auto row = this->_heap.back().reader->read()) {
            this->_heap.back().row = *row;
            std::push_heap(this->_heap.begin(), this->_heap.end(),
                [this](const HeapEntry& e1, const HeapEntry& e2) {
                    return this->_compare(e2.row, e1.row);
                });
        } else {
            this->_heap.pop_back();
        }
    }

public:
    explicit MergeReader(Compare compare): _compare(std::move(compare)) {}

    void add_reader(std::unique_ptr<GeoRowReader> reader) {
        this->_heap.emplace_back();
        this->_heap.back().reader = std::move(reader);
        this->advance();
    }

    virtual std::optional<GeoRow> read() override {
        if (this->_heap.empty()) {
            return {};
        }

        GeoRow row = this->_heap.front().row;
        std::pop_heap(this->_heap.begin(), this->_heap.end(),
            [this](const HeapEntry& e1, const HeapEntry& e2) {
                return this->_compare(e2.row, e1.row);
            });
        this->advance();
        return row;
    }
};

}
