#pragma once
#include <string>
#include <vector>
#include <map>
#include "geo.h"

namespace domain {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;  
    std::map<std::string, int> road_distances;
};

struct Bus {
    std::string name;
    std::vector<std::string> stops;
    bool is_roundtrip;
};

} 