#include "geosick/sampler.hpp"
#include "geosick/geo_distance.hpp"
#include <algorithm>
#include <cassert>

namespace geosick {

// Maximum allowable time duration for interpolation between two points
static constexpr int32_t MAX_DELTA_TIME = 5*60;

// Maximum allowable distance for interpolation in meters
static constexpr double MAX_DELTA_DISTANCE_M = 100;
static constexpr double MAX_DELTA_DISTANCE_M_POW2 = MAX_DELTA_DISTANCE_M * MAX_DELTA_DISTANCE_M;

// Minimum allowable accuracy in meters.
static constexpr uint16_t MIN_ACCURACY_M = 4;

namespace {

int round_up(int num_to_round, int multiple)
{
    assert(multiple != 0);

    int remainder = abs(num_to_round) % multiple;
    if (remainder == 0) {
        return num_to_round;
    }

    if (num_to_round < 0) {
        return -(abs(num_to_round) - remainder);
    } else {
        return num_to_round + multiple - remainder;
    }
}

template<typename T>
T integer_mod(T x, T m) {
    return ((x % m) + m) % m;
}

} // END OF ANONYMOUS NAMESPACE


Sampler::Sampler(int32_t begin_time, int32_t end_time, int32_t period)
 : m_begin_time(begin_time), m_end_time(end_time), m_end_offset(end_time - begin_time),
   m_period(period)
{
    assert(m_begin_time <= m_end_time);
}

int32_t Sampler::time_index_to_timestamp(int32_t time_index) const {
    return m_begin_time + m_period * time_index;
}

void
Sampler::sample(ArrayView<const GeoRow> rows, std::vector<GeoSample>& out_samples) const
{
    assert(std::is_sorted(rows.begin(), rows.end(),
        [](const auto& lhs, const auto& rhs) {
            return lhs.timestamp_utc_s < rhs.timestamp_utc_s;
        }
    ));

    std::vector<GeoSample> samples;
    for (size_t i = 1; i < rows.size(); ++i) {
        const auto& row = rows.at(i - 1);
        const auto& next_row = rows.at(i);
        assert(row.user_id == next_row.user_id);

        int32_t row_timestamp = row.timestamp_utc_s;
        int32_t next_row_timestamp = next_row.timestamp_utc_s;
        int32_t time_delta = next_row_timestamp - row_timestamp;
        double distance_m_pow2 = pow2_geo_distance_fast_m(
            row.lat, row.lon, next_row.lat, next_row.lon);

        if (row_timestamp > m_end_time) {
            break;
        } else if (next_row_timestamp < m_begin_time) {
            continue; // TODO: Possibly use binary search
        } else if (time_delta > MAX_DELTA_TIME) {
            continue;
        } else if (distance_m_pow2 > MAX_DELTA_DISTANCE_M_POW2) {
            continue;
        }

        int32_t row_offset = row_timestamp - m_begin_time;
        int32_t next_row_offset = next_row_timestamp - m_begin_time;
        assert(next_row_offset >= 0);

        int32_t offset = round_up(row_offset, m_period);
        if (offset < 0) {
            offset = integer_mod(offset, m_period);
        }

        for (; offset < next_row_offset && offset <= m_end_offset; offset += m_period) {
            out_samples.push_back(
                get_weighted_sample(row, next_row, row_offset, next_row_offset, offset)
            );
        }
    }
}

GeoSample
Sampler::get_weighted_sample(const GeoRow& row, const GeoRow& next_row,
        int32_t row_offset, int32_t next_row_offset, int32_t offset) const
{
    assert(offset >= 0);
    assert(offset % m_period == 0);
    assert(row_offset <= offset);
    assert(offset <= next_row_offset);

    auto spread = next_row_offset - row_offset;
    double w1 = 1 - double(offset - row_offset) / spread;
    double w2 = 1 - w1;
    assert(0 <= w1 && w1 <= 1);
    assert(0 <= w2 && w2 <= 1);

    return GeoSample{
        .time_index = int32_t(offset / m_period),
        .user_id = row.user_id,
        .lat = int32_t(w1*row.lat + w2*next_row.lat),
        .lon = int32_t(w1*row.lon + w2*next_row.lon),
        .accuracy_m = std::max(MIN_ACCURACY_M, uint16_t(w1*row.accuracy_m + w2*next_row.accuracy_m)),
    };
}

}
