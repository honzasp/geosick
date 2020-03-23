#pragma once
#include "geosick/sampler.hpp"

namespace geosick {

class GeoSearch {
public:
    struct Rect {
        int32_t lat_begin, lat_end;
        int32_t lon_begin, lon_end;

    };

    explicit GeoSearch(const std::vector<GeoSample>& samples);
    std::vector<GeoSample::UserID> search_in_rectangle(const Rect& rect) const;
};

}
