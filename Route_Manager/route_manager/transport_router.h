#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <optional>

namespace transport {

struct RoutingSettings {
    int bus_wait_time = 0;
    double bus_velocity = 0.0;
};

struct RouteEdgeInfo {
    std::string bus_name; 
    int span_count = 0; 
    double travel_time = 0.0; 
};

struct RouteItem {
    std::string type;
    std::string stop_name;
    std::string bus_name;
    int span_count = 0;         
    double time = 0.0;          
};

struct RouteInfo {
    double total_time = 0.0;    
    std::vector<RouteItem> items; 
};

class TransportRouter {
public:
    TransportRouter(const TransportCatalogue& catalogue, const RoutingSettings& settings);
    
    TransportRouter(const TransportRouter&) = delete;
    TransportRouter& operator=(const TransportRouter&) = delete;
    
    TransportRouter(TransportRouter&&) = default;
    TransportRouter& operator=(TransportRouter&&) = default;
    
    std::optional<RouteInfo> BuildRoute(const std::string& from, const std::string& to) const;
    
    bool IsInitialized() const { return router_ != nullptr; }

private:
    const TransportCatalogue& catalogue_;
    RoutingSettings settings_;
    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    std::unordered_map<std::string, graph::VertexId> stop_to_vertex_;
    std::vector<std::string> vertex_to_stop_;
    std::vector<RouteEdgeInfo> edges_info_;

    void BuildGraph();
    void AddBusToGraph(const domain::Bus& bus);
    double CalculateTravelTime(const std::string& from, const std::string& to) const;
};

} // namespace transport
