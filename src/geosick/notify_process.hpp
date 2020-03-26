#pragma once
#include "geosick/match.hpp"

namespace geosick {

class NotifyProcess {
public:
    void notify(const Match& match);
};

}
