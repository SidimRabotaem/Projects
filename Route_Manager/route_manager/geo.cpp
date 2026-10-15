#define _USE_MATH_DEFINES 
#include "geo.h" 

#include <cmath> 
#include <iostream> 

namespace geo { 

const double EARTH_RADIUS = 6371000.0;

double ComputeDistance(Coordinates from, Coordinates to) { 
    using namespace std; 
     
    const double dr = M_PI / 180.0; 
     
    double lat1 = from.lat * dr; 
    double lon1 = from.lng * dr; 
    double lat2 = to.lat * dr; 
    double lon2 = to.lng * dr; 
     
    double dlat = lat2 - lat1; 
    double dlon = lon2 - lon1; 
     
    double a = sin(dlat/2) * sin(dlat/2) + 
               cos(lat1) * cos(lat2) * sin(dlon/2) * sin(dlon/2); 
    double c = 2 * atan2(sqrt(a), sqrt(1-a)); 
     
    double distance = EARTH_RADIUS * c; 
     
    return distance; 
} 

} 