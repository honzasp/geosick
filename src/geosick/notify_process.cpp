#include <iostream>
#include "geosick/notify_process.hpp"

namespace geosick {

void NotifyProcess::notify(const Match& match) {
    std::cout << "match " << match.query_user_id
        << " x " << match.sick_user_id
        << ", score " << match.score
        << ", distance " << match.min_distance_m
        << std::endl;
}

}
