#include "geosick/config.hpp"
#include "geosick/slice.hpp"

namespace geosick {

struct GeoRow;
struct GeoSample;

struct Match {
    uint32_t query_user_id;
    uint32_t sick_user_id;
    ArrayView<const GeoRow> query_rows;
    ArrayView<const GeoRow> sick_rows;
    ArrayView<const GeoSample> query_samples;
    ArrayView<const GeoSample> sick_samples;

    void evaluate(const Config& cfg);

    double score;
    double min_distance_m;
};

}
