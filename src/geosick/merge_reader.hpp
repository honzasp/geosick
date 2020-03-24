#pragma once
#include <algorithm>
#include <vector>
#include "geosick/geo_row_reader.hpp"

namespace geosick {

template<class Compare>
class MergeReader final: public GeoRowReader {
    Compare m_compare;
    
    struct HeapEntry {
        GeoRow row;
        std::unique_ptr<GeoRowReader> reader;
    };
    std::vector<HeapEntry> m_heap;

    void advance() {
        if (auto row = m_heap.back().reader->read()) {
            m_heap.back().row = *row;
            std::push_heap(m_heap.begin(), m_heap.end(),
                [this](const HeapEntry& e1, const HeapEntry& e2) {
                    return m_compare(e2.row, e1.row);
                });
        } else {
            m_heap.pop_back();
        }
    }

public:
    explicit MergeReader(Compare compare): m_compare(std::move(compare)) {}

    void add_reader(std::unique_ptr<GeoRowReader> reader) {
        m_heap.emplace_back();
        m_heap.back().reader = std::move(reader);
        this->advance();
    }

    size_t get_reader_count() const {
        return m_heap.size();
    }

    virtual std::optional<GeoRow> read() override {
        if (m_heap.empty()) {
            return {};
        }

        GeoRow row = m_heap.front().row;
        std::pop_heap(m_heap.begin(), m_heap.end(),
            [this](const HeapEntry& e1, const HeapEntry& e2) {
                return m_compare(e2.row, e1.row);
            });
        this->advance();
        return row;
    }
};

}
