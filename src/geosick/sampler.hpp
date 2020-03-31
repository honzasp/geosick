#pragma once
#include <vector>
#include <chrono>
#include "geosick/geo_row.hpp"
#include "geosick/slice.hpp"

namespace geosick {

struct GeoSample {
    using UserID = uint32_t;
    int32_t time_index;
    UserID user_id;
    int32_t lat;
    int32_t lon;
    uint16_t accuracy_m;
    // TODO: velocity_n, velocity_e
};

class Sampler {
private:
    // Maximum allowable time duration for interpolation between two points
    static constexpr int32_t MAX_DELTA_TIME = 5*60;
    // Maximum allowable distance for interpolation in meters
    static constexpr double MAX_DELTA_DISTANCE_M = 100;
    static constexpr double MAX_DELTA_DISTANCE_M_POW2 = MAX_DELTA_DISTANCE_M * MAX_DELTA_DISTANCE_M;

    int32_t m_begin_time;
    int32_t m_end_time;
    int32_t m_end_offset;
    int32_t m_period;

public:
    explicit Sampler(int32_t begin_time, int32_t end_time, int32_t period_s);

    int32_t time_index_to_timestamp(int32_t time_index) const;

    void
    sample(ArrayView<const GeoRow> rows, std::vector<GeoSample>& out_samples) const;

private:
    GeoSample get_weighted_sample(const GeoRow& row, const GeoRow& next_row,
        int32_t row_offset, int32_t next_row_offset, int32_t offset) const;
};

}
