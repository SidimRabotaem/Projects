#include "transport_router.h"
#include "transport_catalogue.h"

namespace transport {

TransportRouter::TransportRouter(const TransportCatalogue& catalogue, const RoutingSettings& settings)
    : catalogue_(catalogue)
    , settings_(settings)
{
    BuildGraph();
    router_ = std::make_unique<graph::Router<double>>(graph_);
}

std::optional<RouteInfo> TransportRouter::BuildRoute(const std::string& from, const std::string& to) const {
    if (!router_) {
        return std::nullopt;
    }

    auto from_it = stop_to_vertex_.find(from);
    auto to_it = stop_to_vertex_.find(to);

    if (from_it == stop_to_vertex_.end() || to_it == stop_to_vertex_.end()) {
        return std::nullopt;
    }

    auto route = router_->BuildRoute(from_it->second, to_it->second);
    if (!route) {
        return std::nullopt;
    }
    
    RouteInfo result;
    result.total_time = route->weight;
    
    for (auto edge_id : route->edges) {
        const auto& edge = graph_.GetEdge(edge_id);
        const auto& edge_info = edges_info_[edge_id];
        
        std::string wait_stop_name = vertex_to_stop_[edge.from];
        result.items.push_back(RouteItem{
            "Wait",
            wait_stop_name,
            "",
            0,
            static_cast<double>(settings_.bus_wait_time)
        });
        
        result.items.push_back(RouteItem{
            "Bus",
            "",
            edge_info.bus_name,
            edge_info.span_count,
            edge_info.travel_time
        });
    }
    
    return result;
}

void TransportRouter::BuildGraph() {
    const auto& all_stops = catalogue_.GetStops();
    
    size_t vertex_count = all_stops.size();
    graph_ = graph::DirectedWeightedGraph<double>(vertex_count);
    
    for (const auto& stop : all_stops) {
        graph::VertexId vertex_id = vertex_to_stop_.size();
        stop_to_vertex_[stop.name] = vertex_id;
        vertex_to_stop_.push_back(stop.name);
    }
    
    const auto& all_buses = catalogue_.GetBuses();
    for (const auto& bus : all_buses) {
        AddBusToGraph(bus);
    }
    
    edges_info_.reserve(graph_.GetEdgeCount());
}

void TransportRouter::AddBusToGraph(const domain::Bus& bus) {
    std::vector<std::string> stops = bus.stops;
    if (stops.size() < 2) return;

    if (!bus.is_roundtrip) {
        for (size_t i = bus.stops.size() - 1; i-- > 0; ) {
            stops.push_back(bus.stops[i]);
        }
    }

    for (size_t i = 0; i < stops.size(); ++i) {
        double total_distance = 0;
        for (size_t j = i + 1; j < stops.size(); ++j) {
            auto from_stop = catalogue_.FindStop(stops[j - 1]);
            auto to_stop = catalogue_.FindStop(stops[j]);
            if (!from_stop || !to_stop) {
                total_distance = 0;
                break;
            }
            total_distance += catalogue_.GetDistance(from_stop, to_stop);
            if (total_distance <= 0) continue;

            double travel_time = total_distance / (settings_.bus_velocity * 1000.0 / 60.0);
            double total_time = settings_.bus_wait_time + travel_time;

            graph::Edge<double> edge{
                stop_to_vertex_.at(stops[i]),
                stop_to_vertex_.at(stops[j]),
                total_time
            };
            graph_.AddEdge(edge);
            edges_info_.push_back({
                bus.name,
                static_cast<int>(j - i),
                travel_time
            });
        }
    }
}

double TransportRouter::CalculateTravelTime(const std::string& from, const std::string& to) const {
    domain::Stop* from_stop = catalogue_.FindStop(from);
    domain::Stop* to_stop = catalogue_.FindStop(to);
    
    if (!from_stop || !to_stop) {
        return 0.0;
    }
    
    int distance = catalogue_.GetDistance(from_stop, to_stop);
    
    if (distance == 0) {
        return 0.0;
    }
    
    double speed_m_per_min = settings_.bus_velocity * 1000.0 / 60.0;
    return distance / speed_m_per_min;
}

} // namespace transport