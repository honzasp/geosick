#include <vector>
#include "geosick/config.hpp"
#include "geosick/slice.hpp"

namespace geosick {

struct GeoRow;
struct GeoSample;

struct MatchInput {
    uint32_t query_user_id;
    uint32_t sick_user_id;
    ArrayView<const GeoRow> query_rows;
    ArrayView<const GeoRow> sick_rows;
    ArrayView<const GeoSample> query_samples;
    ArrayView<const GeoSample> sick_samples;
};

struct MatchStep {
    int32_t time_index;
    double infect_rate;
    double distance_m;
};

struct MatchOutput {
    double score;
    double min_distance_m;
    std::vector<MatchStep> steps;
};

MatchOutput evaluate_match(const Config& cfg, const MatchInput& input);

}
