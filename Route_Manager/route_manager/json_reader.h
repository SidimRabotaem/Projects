#pragma once
#include "transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "svg.h"
#include "transport_router.h"

namespace json_reader {

class JsonReader {
public:
    void LoadData(TransportCatalogue& catalogue, const json::Document& input);
    json::Document ProcessRequests(const TransportCatalogue& catalogue, const json::Document& input, const transport::TransportRouter& router);
    const transport::RoutingSettings& GetRoutingSettings() const { 
        return routing_settings_;
    }

private:
    transport::RoutingSettings routing_settings_;
    map_renderer::MapRenderer map_renderer_;
    std::unique_ptr<transport::TransportRouter> router_;

    domain::Stop ParseStop(const json::Dict& dict);
    domain::Bus ParseBus(const json::Dict& dict);
    map_renderer::RenderSettings ParseRenderSettings(const json::Dict& dict);
    svg::Color ParseColor(const json::Node& node);
    json::Node ProcessBusRequest(const TransportCatalogue& catalogue, const json::Dict& request_map);
    json::Node ProcessStopRequest(const TransportCatalogue& catalogue, const json::Dict& request_map);
    json::Node ProcessRouteRequest(const TransportCatalogue& catalogue, const json::Dict& request_map, const transport::TransportRouter& router);
};

} // namespace json_reader