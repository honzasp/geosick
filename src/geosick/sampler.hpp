#pragma once
#include <vector>
#include <chrono>
#include "geosick/geo_row.hpp"
#include "geosick/slice.hpp"

namespace geosick {

struct GeoSample {
    using UserID = uint32_t;
    uint32_t time_index;
    UserID user_id;
    int32_t lat;
    int32_t lon;
    uint16_t accuracy_m;
};

using DurationS = std::chrono::duration<int32_t, std::ratio<1>>;
using UtcTime = std::chrono::time_point<std::chrono::steady_clock, DurationS>;

class Sampler {
private:
    // Maximum allowable time duration for interpolation between two points
    static constexpr DurationS MAX_DELTA_TIME = std::chrono::minutes(5);
    // Maximum allowable distance for interpolation in meters
    static constexpr unsigned MAX_DELTA_DISTANCE_M = 100;
    static constexpr unsigned MAX_DELTA_DISTANCE_M_POW2 = MAX_DELTA_DISTANCE_M * MAX_DELTA_DISTANCE_M;

    UtcTime m_begin_time;
    UtcTime m_end_time;
    DurationS m_end_offset;
    DurationS m_period;

public:
    explicit Sampler(UtcTime begin_time, UtcTime end_time, DurationS period_s);

    uint32_t get_max_time_index() const;

    void
    sample(ArrayView<const GeoRow> rows, std::vector<GeoSample>& out_samples) const;

private:
    GeoSample get_weighted_sample(const GeoRow& row, const GeoRow& next_row,
        DurationS row_offset, DurationS next_row_offset, DurationS offset) const;
};

}
