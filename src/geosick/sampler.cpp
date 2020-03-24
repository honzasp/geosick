#include "sampler.hpp"
#include <algorithm>
#include <cassert>

namespace geosick {
namespace {

int roundUp(int numToRound, int multiple)
{
    assert(multiple != 0);

    int remainder = abs(numToRound) % multiple;
    if (remainder == 0) {
        return numToRound;
    }

    if (numToRound < 0) {
        return -(abs(numToRound) - remainder);
    } else {
        return numToRound + multiple - remainder;
    }
}

} // END OF ANONYMOUS NAMESPACE


Sampler::Sampler(UtcTime begin_time, UtcTime end_time, DurationS period)
 : m_begin_time(begin_time), m_end_time(end_time), m_period(period)
{
    assert(m_begin_time <= m_end_time);
}

std::vector<GeoSample>
Sampler::sample(const std::vector<GeoRow>& rows) const
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

        if (row_timestamp > m_end_time) {
            break;
        } else if (next_row_timestamp < m_begin_time) {
            continue; // TODO: Possibly use binary search
        }

        auto row_offset = row_timestamp - m_begin_time;
        auto next_row_offset = row_timestamp - m_begin_time;
        assert(next_row_offset > DurationS(0));

        auto offset = DurationS(roundUp(row_offset.count(), m_period.count()));
        while (offset < DurationS(0)) {
            offset += m_period;
        }

        for (; offset < next_row_offset; offset += m_period) {
            samples.push_back(get_weighted_sample(row, next_row, row_offset, next_row_offset, offset));
        }
    }
    return samples;
}

GeoSample
Sampler::get_weighted_sample(const GeoRow& row, const GeoRow& next_row,
        DurationS row_offset, DurationS next_row_offset, DurationS offset) const
{
    assert(offset > DurationS(0));
    assert(offset % m_period == DurationS(0));
    assert(row_offset <= offset);
    assert(offset <= next_row_offset);

    auto spread = next_row_offset - row_offset;
    double w1 = double((offset - row_offset).count()) / spread.count();
    double w2 = 1 - w1;
    assert(0 <= w1 && w1 <= 1);
    assert(0 <= w2 && w2 <= 1);

    return GeoSample{
        .time_frame_id = uint32_t(offset / m_period),
        .user_id = row.user_id,
        .lat = int32_t(w1*row.lat + w2*next_row.lat),
        .lon = int32_t(w1*row.lon + w2*next_row.lon),
        .accuracy_m = uint16_t(w1*row.accuracy_m + w2*next_row.accuracy_m)
    };
}


}
