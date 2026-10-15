#pragma once 

#include <string> 
#include <vector> 
#include <unordered_map> 
#include <map> 
#include <set> 
#include <algorithm> 
#include "domain.h" 
#include "geo.h" 
#include <deque> 

class TransportCatalogue { 
public: 

    const std::deque<domain::Stop>& GetStops() const { 
        return stops_; 
    } 

    const std::deque<domain::Bus>& GetBuses() const { 
        return buses_; 
    } 
     
    struct BusInfo { 
        size_t stops_count = 0; 
        size_t unique_stops_count = 0; 
        double route_length = 0.0; 
        double curvature = 0.0;  
        bool found = false; 
    }; 

    domain::Stop* AddStop(std::string name, geo::Coordinates coordinates, std::map<std::string, int> road_distances); 
    domain::Bus* AddBusRoute(std::string, bool is_roundtrip, const std::vector<std::string>& stop_names); 

    BusInfo GetBusInfo(const std::string& bus_number) const; 
    const std::set<std::string>& GetBusesForStop(const std::string& stop_name) const; 

    void SetDistance(const std::string& from, const std::string& to, int distance); 
    void SetDistance(domain::Stop* from, domain::Stop* to, int distance); 
    domain::Stop* FindStop(const std::string& name) const; 
    domain::Bus* FindBusRoute(const std::string& route_number) const; 
    int GetDistance(domain::Stop* from, domain::Stop* to) const; 

private: 
    std::deque<domain::Stop> stops_; 
    std::deque<domain::Bus> buses_; 

    std::map<std::string, domain::Stop*> stop_name_to_stop_; 
    std::map<std::string, domain::Bus*> bus_number_to_route_; 
    std::map<const domain::Stop*, std::set<std::string>> stop_to_buses_; 
    std::map<std::pair<const domain::Stop*, const domain::Stop*>, int> distances_; 
    std::map<std::string, std::vector<std::pair<std::string, int>>> pending_distances_;
}; 