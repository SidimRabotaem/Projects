#include "transport_catalogue.h"
#include "geo.h"
#include <unordered_set>
#include <cmath>
#include <iostream>

using namespace std;

domain::Stop* TransportCatalogue::AddStop(std::string name, geo::Coordinates coordinates, std::map<std::string, int> road_distances) {
    stops_.push_back({std::move(name), coordinates, {}}); 
    domain::Stop* stop_ptr = &stops_.back();
    stop_name_to_stop_[stop_ptr->name] = stop_ptr;

    for (const auto& [neighbor_name, distance] : road_distances) {
        domain::Stop* neighbor_stop = FindStop(neighbor_name);
        if (neighbor_stop) {
            distances_[{stop_ptr, neighbor_stop}] = distance;
        } else {
            pending_distances_[stop_ptr->name].push_back({neighbor_name, distance});
        }
    }

    if (pending_distances_.count(stop_ptr->name)) {
        for (const auto& [to_name, dist] : pending_distances_[stop_ptr->name]) {
            if (auto to_stop = FindStop(to_name)) {
                distances_[{stop_ptr, to_stop}] = dist;
            }
        }
        pending_distances_.erase(stop_ptr->name);
    }

    return stop_ptr;
}

domain::Bus* TransportCatalogue::AddBusRoute(std::string name, bool is_roundtrip, const std::vector<std::string>& stop_names) {
    buses_.push_back({name, stop_names, is_roundtrip});
    domain::Bus* new_bus_ptr = &buses_.back();
    bus_number_to_route_[new_bus_ptr->name] = new_bus_ptr;

    for (const auto& stop_name : stop_names) {
        if (auto stop_ptr = FindStop(stop_name)) {
            stop_to_buses_[stop_ptr].insert(new_bus_ptr->name);
        }
    }

    return new_bus_ptr;
}

domain::Stop* TransportCatalogue::FindStop(const std::string& name) const {
    if (auto it = stop_name_to_stop_.find(name); it != stop_name_to_stop_.end()) {
        return it->second;
    }
    return nullptr;
}

domain::Bus* TransportCatalogue::FindBusRoute(const std::string& route_number) const {
    if (auto it = bus_number_to_route_.find(route_number); it != bus_number_to_route_.end()) {
        return it->second;
    }
    return nullptr;
}

int TransportCatalogue::GetDistance(domain::Stop* from, domain::Stop* to) const {
    if (!from || !to) return 0;

    if (auto it = distances_.find({from, to}); it != distances_.end()) {
        return it->second;
    }
    if (auto it = distances_.find({to, from}); it != distances_.end()) {
        return it->second;
    }
    return static_cast<int>(geo::ComputeDistance(from->coordinates, to->coordinates));
}

void TransportCatalogue::SetDistance(const std::string& from, const std::string& to, int distance) {
    domain::Stop* from_stop = FindStop(from);
    domain::Stop* to_stop = FindStop(to);
    if (from_stop && to_stop) {
        distances_[{from_stop, to_stop}] = distance;
    }
}

void TransportCatalogue::SetDistance(domain::Stop* from, domain::Stop* to, int distance) {
    if (from && to) {
        distances_[{from, to}] = distance;
    }
}

TransportCatalogue::BusInfo TransportCatalogue::GetBusInfo(const std::string& bus_name) const {
    BusInfo info;
    const auto* bus = FindBusRoute(bus_name);
    if (!bus) {
        info.found = false;
        return info;
    }

    info.found = true;
    info.stops_count = bus->is_roundtrip ? bus->stops.size()
                                         : bus->stops.size() * 2 - 1;

    std::set<std::string> unique_stops(bus->stops.begin(), bus->stops.end());
    info.unique_stops_count = unique_stops.size();

    double geo_length = 0.0;
    int road_length = 0;

    for (size_t i = 1; i < bus->stops.size(); ++i) {
        auto* stop1 = FindStop(bus->stops[i - 1]);
        auto* stop2 = FindStop(bus->stops[i]);
        if (!stop1 || !stop2) continue;

        geo_length += geo::ComputeDistance(stop1->coordinates, stop2->coordinates);
        road_length += GetDistance(stop1, stop2);
    }

    if (!bus->is_roundtrip) {
        for (size_t i = bus->stops.size() - 1; i > 0; --i) {
            auto* stop1 = FindStop(bus->stops[i]);
            auto* stop2 = FindStop(bus->stops[i - 1]);
            if (!stop1 || !stop2) continue;

            geo_length += geo::ComputeDistance(stop1->coordinates, stop2->coordinates);
            road_length += GetDistance(stop1, stop2);
        }
    }

    info.route_length = road_length;
    info.curvature = road_length / geo_length;

    return info;
}

const std::set<std::string>& TransportCatalogue::GetBusesForStop(const std::string& stop_name) const {
    static const std::set<std::string> empty_result;

    domain::Stop* stop_ptr = FindStop(stop_name);
    if (!stop_ptr) {
        return empty_result;
    }

    if (auto it = stop_to_buses_.find(stop_ptr); it != stop_to_buses_.end()) {
        return it->second;
    }
    return empty_result;
}