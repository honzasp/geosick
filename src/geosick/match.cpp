#include <cassert>
#include <cmath>
#include <iostream>
#include "geosick/geo_distance.hpp"
#include "geosick/sampler.hpp"
#include "geosick/match.hpp"

namespace geosick {

static const double PI = 3.14159265359;
static const double INFECT_RADIUS = 2.0;
static const double INFECT_MAX_SPEED = 3.0;
static const double INFECT_RATE = 0.5/60.0;

static double circle_isect_area(double r1, double r2, double d) {
    if (r1 + r2 < d) { return 0.0; }
    if (d + r1 <= r2) { return PI * r1*r1; }
    if (d + r2 <= r1) { return PI * r2*r2; }

    double d1 = (d*d - r2*r2 + r1*r1) / (2*d);
    double d2 = d - d1;
    double a = std::sqrt((r1+r2+d)*(r1+r2-d)*(r1-r2-d)*(r2-r1-d)) / d;
    double theta1 = 2*std::acos(d1/r1);
    double theta2 = 2*std::acos(d2/r2);
    return 0.5*(theta1*r1*r1 + theta2*r2*r2 - a*d);
}

static double eval_infect_rate(
    const GeoSample& query, const GeoSample& sick, double distance)
{
    double query_radius = double(query.accuracy_m);
    double sick_radius = double(sick.accuracy_m);
    if (distance >= query_radius + sick_radius) { return 0.0; }

    double sick_area = PI * sick_radius * sick_radius;
    double query_area = PI * query_radius * query_radius;
    double isect_area = circle_isect_area(query_radius, sick_radius, distance);
    double infect_area = std::min(PI * INFECT_RADIUS*INFECT_RADIUS, isect_area);

    return INFECT_RATE * (infect_area*isect_area) / (sick_area*query_area);
}

MatchOutput evaluate_match(const Config& cfg, const MatchInput& input) {
    double compl_score_log = 0.0;
    double min_distance = std::numeric_limits<double>::infinity();

    size_t query_i = 0;
    size_t sick_i = 0;
    MatchOutput output;
    while (query_i < input.query_samples.size() && sick_i < input.sick_samples.size()) {
        const auto& query_sample = input.query_samples.at(query_i);
        const auto& sick_sample = input.sick_samples.at(sick_i);
        if (query_sample.time_index < sick_sample.time_index) {
            ++query_i; continue;
        } else if (query_sample.time_index > sick_sample.time_index) {
            ++sick_i; continue;
        } else {
            ++query_i; ++sick_i;
        }

        MatchStep step;
        step.time_index = query_sample.time_index;
        step.distance_m = std::sqrt(pow2_geo_distance_fast_m(
            query_sample.lat, query_sample.lon,
            sick_sample.lat, sick_sample.lon));
        step.infect_rate = eval_infect_rate(query_sample, sick_sample, step.distance_m);
        assert(std::isfinite(step.infect_rate));
        output.steps.push_back(step);

        if (step.infect_rate > 0.0) {
            compl_score_log += std::log1p(
                -std::min(0.9, double(cfg.period_s)*step.infect_rate));
        }
        min_distance = std::min(min_distance, step.distance_m +
            0.5*double(query_sample.accuracy_m) + 0.5*double(sick_sample.accuracy_m));
    }

    output.score = 0.0 - std::expm1(compl_score_log);
    output.min_distance_m = min_distance;
    return output;
}

}
