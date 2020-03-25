#include "geosick/sampler.hpp"
#include "geosick/geo_distance.hpp"
#include <algorithm>
#include <cassert>

namespace geosick {
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


Sampler::Sampler(UtcTime begin_time, UtcTime end_time, DurationS period)
 : m_begin_time(begin_time), m_end_time(end_time), m_end_offset(end_time - begin_time),
   m_period(period)
{
    assert(m_begin_time <= m_end_time);
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

        const auto row_timestamp = UtcTime(DurationS(row.timestamp_utc_s));
        const auto next_row_timestamp = UtcTime(DurationS(next_row.timestamp_utc_s));
        const auto time_delta = next_row_timestamp - row_timestamp;
        const auto distance_m_pow2 = pow2_geo_distance_fast_m(row.lat, row.lon, next_row.lat, next_row.lon);

        if (row_timestamp > m_end_time) {
            break;
        } else if (next_row_timestamp < m_begin_time) {
            continue; // TODO: Possibly use binary search
        } else if (time_delta > MAX_DELTA_TIME) {
            continue;
        } else if (distance_m_pow2 > MAX_DELTA_DISTANCE_M_POW2) {
            continue;
        }

        auto row_offset = row_timestamp - m_begin_time;
        auto next_row_offset = next_row_timestamp - m_begin_time;
        assert(next_row_offset > DurationS::zero());

        auto offset = DurationS(round_up(row_offset.count(), m_period.count()));
        if (offset < DurationS::zero()) {
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
        DurationS row_offset, DurationS next_row_offset, DurationS offset) const
{
    assert(offset >= DurationS::zero());
    assert(offset % m_period == DurationS::zero());
    assert(row_offset <= offset);
    assert(offset <= next_row_offset);

    auto spread = next_row_offset - row_offset;
    double w1 = 1 - double((offset - row_offset).count()) / spread.count();
    double w2 = 1 - w1;
    assert(0 <= w1 && w1 <= 1);
    assert(0 <= w2 && w2 <= 1);

    return GeoSample{
        .time_index = uint32_t(offset / m_period),
        .user_id = row.user_id,
        .lat = int32_t(w1*row.lat + w2*next_row.lat),
        .lon = int32_t(w1*row.lon + w2*next_row.lon),
        .accuracy_m = uint16_t(w1*row.accuracy_m + w2*next_row.accuracy_m)
    };
}

uint32_t
Sampler::get_max_time_index() const
{
    return m_end_offset / m_period;
}

}
