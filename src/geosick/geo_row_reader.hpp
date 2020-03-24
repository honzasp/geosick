#pragma once
#include <optional>
#include "geosick/geo_row.hpp"

namespace geosick {

class GeoRowReader {
public:
    virtual ~GeoRowReader() {}
    virtual std::optional<GeoRow> read() = 0;
};

}
