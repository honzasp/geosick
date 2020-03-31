#pragma once
#include <cstdint>

namespace geosick {

struct GeoRow {
    uint32_t user_id;
    int32_t timestamp_utc_s;
    int32_t lat;
    int32_t lon;
    uint16_t accuracy_m;
    uint16_t altitude_m = UINT16_MAX;
    uint16_t heading_deg = UINT16_MAX;
    int32_t velocity_mps = 0;
};

}
