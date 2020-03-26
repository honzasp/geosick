#pragma once
#include <unordered_map>
#include <vector>
#include "geosick/slice.hpp"

namespace geosick {

struct SickMap {
    std::vector<GeoRow> rows;
    std::vector<GeoSample> samples;
    std::unordered_map<uint32_t, size_t> user_id_to_idx;
    std::vector<size_t> row_offsets;
    std::vector<size_t> sample_offsets;

    ArrayView<const GeoRow> rows_by_idx(size_t idx) const {
        size_t begin = this->row_offsets.at(idx);
        size_t end = this->row_offsets.at(idx + 1);
        return {this->rows.data() + begin, this->rows.data() + end};
    }

    ArrayView<const GeoSample> samples_by_idx(size_t idx) const {
        size_t begin = this->sample_offsets.at(idx);
        size_t end = this->sample_offsets.at(idx + 1);
        return {this->samples.data() + begin, this->samples.data() + end};
    }
};

}
