#pragma once
#include <vector>
#include "geosick/geo_row.hpp"

namespace geosick {

struct GeoSample {
    using UserID = uint32_t;

    ...
};


class Sampler {

    explicit Sampler(uint32_t begin_time, uint32_t end_time, uint32_t period);

    uint32_t get_max_time_index() const;

    std::vector<GeoSample> sample(const std::vector<GeoRow>& rows) const;

};

}
