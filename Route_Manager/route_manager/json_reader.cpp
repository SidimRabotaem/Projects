#include "json_reader.h"
#include "map_renderer.h"
#include "transport_router.h"
#include <sstream>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace json;
using namespace json_reader;

void JsonReader::LoadData(TransportCatalogue& catalogue, const Document& input) {

    const auto& root = input.GetRoot().AsMap();

    if (root.count("render_settings")) {
        map_renderer_.SetSettings(ParseRenderSettings(root.at("render_settings").AsMap()));
    }

    if (root.count("routing_settings")) {
        const auto& routing_dict = root.at("routing_settings").AsMap();
        routing_settings_.bus_wait_time = routing_dict.at("bus_wait_time").AsInt();
        routing_settings_.bus_velocity = routing_dict.at("bus_velocity").AsDouble();
    }

    if (!root.count("base_requests")) return;

    const auto& base_requests = root.at("base_requests").AsArray();

    for (const auto& node : base_requests) {
        const auto& dict = node.AsMap();
        if (dict.at("type").AsString() == "Stop") {
            const string& name = dict.at("name").AsString();
            double lat = dict.at("latitude").AsDouble();
            double lon = dict.at("longitude").AsDouble();
            catalogue.AddStop(name, {lat, lon}, {});
        }
    }

    for (const auto& node : base_requests) {
        const auto& dict = node.AsMap();
        if (dict.at("type").AsString() == "Stop" && dict.count("road_distances")) {
            const string& name = dict.at("name").AsString();
            for (const auto& [to, distance] : dict.at("road_distances").AsMap()) {
                catalogue.SetDistance(name, to, distance.AsInt());
            }
        }
    }

    for (const auto& node : base_requests) {
        const auto& dict = node.AsMap();
        if (dict.at("type").AsString() == "Bus") {
            domain::Bus bus;
            bus.name = dict.at("name").AsString();
            bus.is_roundtrip = dict.at("is_roundtrip").AsBool();
            for (const auto& stop : dict.at("stops").AsArray()) {
                bus.stops.push_back(stop.AsString());
            }
            catalogue.AddBusRoute(bus.name, bus.is_roundtrip, bus.stops);
        }
    }

    router_ = std::make_unique<transport::TransportRouter>(catalogue, routing_settings_);
}

domain::Stop JsonReader::ParseStop(const Dict& dict) {
    domain::Stop stop;
    stop.name = dict.at("name").AsString();

    const auto& lat_node = dict.at("latitude");
    const auto& lon_node = dict.at("longitude");

    stop.coordinates.lat = lat_node.IsDouble() || lat_node.IsInt()
        ? lat_node.AsDouble()
        : 0.0;
    stop.coordinates.lng = lon_node.IsDouble() || lon_node.IsInt()
        ? lon_node.AsDouble()
        : 0.0;

    if (dict.count("road_distances")) {
        for (const auto& [neighbor, value] : dict.at("road_distances").AsMap()) {
            stop.road_distances[neighbor] = value.AsInt();
        }
    }

    return stop;
}

domain::Bus JsonReader::ParseBus(const Dict& dict) {
    domain::Bus bus;
    bus.name = dict.at("name").AsString();
    bus.is_roundtrip = dict.at("is_roundtrip").AsBool();

    for (const auto& stop : dict.at("stops").AsArray()) {
        bus.stops.push_back(stop.AsString());
    }

    if (bus.stops.empty()) {
        return bus;
    }

    return bus;
}

svg::Color JsonReader::ParseColor(const Node& node) {
    if (node.IsString()) {
        return node.AsString();
    }
    const auto& arr = node.AsArray();
    if (arr.size() == 3) {
        return svg::Rgb(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt());
    } else {
        return svg::Rgba(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt(), arr[3].AsDouble());
    }
}

map_renderer::RenderSettings JsonReader::ParseRenderSettings(const json::Dict& map) {
    map_renderer::RenderSettings settings;

    auto getDouble = [&](const string& key, double def = 0.0) {
        if (map.count(key)) return map.at(key).AsDouble();
        return def;
    };
    auto getInt = [&](const string& key, int def = 0) {
        if (map.count(key)) return map.at(key).AsInt();
        return def;
    };

    settings.width = getDouble("width");
    settings.height = getDouble("height");
    settings.padding = getDouble("padding");
    settings.stop_radius = getDouble("stop_radius");
    settings.line_width = getDouble("line_width");
    settings.bus_label_font_size = getInt("bus_label_font_size");
    settings.stop_label_font_size = getInt("stop_label_font_size");

    if (map.count("bus_label_offset")) {
        const auto& arr = map.at("bus_label_offset").AsArray();
        settings.bus_label_offset = {arr[0].AsDouble(), arr[1].AsDouble()};
    }

    if (map.count("stop_label_offset")) {
        const auto& arr = map.at("stop_label_offset").AsArray();
        settings.stop_label_offset = {arr[0].AsDouble(), arr[1].AsDouble()};
    }

    if (map.count("underlayer_color")) {
        settings.underlayer_color = ParseColor(map.at("underlayer_color"));
    }

    settings.underlayer_width = getDouble("underlayer_width");

    if (map.count("color_palette")) {
        for (const auto& color_node : map.at("color_palette").AsArray()) {
            settings.color_palette.push_back(ParseColor(color_node));
        }
    }

    return settings;
}

json::Document JsonReader::ProcessRequests(const TransportCatalogue& catalogue, const json::Document& input,const transport::TransportRouter& router) {
    
    const auto& root = input.GetRoot().AsMap();
    Array response;

    if (!root.count("stat_requests")) {
        return Document(Node(response));
    }

    for (const auto& request : root.at("stat_requests").AsArray()) {
        const auto& req = request.AsMap();
        const string& type = req.at("type").AsString();

        if (type == "Bus") {
            response.push_back(ProcessBusRequest(catalogue, req));
        } else if (type == "Stop") {
            response.push_back(ProcessStopRequest(catalogue, req));
        } else if (type == "Map") {
            Dict map_resp;
            map_resp["request_id"] = req.at("id").AsInt();
            ostringstream out;
            map_renderer_.RenderMap(catalogue).Render(out);
            map_resp["map"] = out.str();
            response.push_back(map_resp);
        } else if (type == "Route") { 
            response.push_back(ProcessRouteRequest(catalogue, req, router));
        }
    }

    return Document(Node(response));
}

Node JsonReader::ProcessBusRequest(const TransportCatalogue& catalogue, const Dict& req) {
    auto info = catalogue.GetBusInfo(req.at("name").AsString());
    json::Builder builder;
    builder.StartDict().Key("request_id").Value(req.at("id").AsInt());

    if (!info.found) {
        builder.Key("error_message").Value("not found").EndDict();
    } else {
        builder.Key("curvature").Value(info.curvature)
               .Key("route_length").Value(info.route_length)
               .Key("stop_count").Value(static_cast<int>(info.stops_count))
               .Key("unique_stop_count").Value(static_cast<int>(info.unique_stops_count))
               .EndDict();
    }

    return builder.Build();
}

Node JsonReader::ProcessStopRequest(const TransportCatalogue& catalogue, const Dict& req) {
    const auto& stop_name = req.at("name").AsString();
    const auto& buses = catalogue.GetBusesForStop(stop_name);

    json::Builder builder;
    builder.StartDict().Key("request_id").Value(req.at("id").AsInt());

    if (buses.empty()) {
        if (catalogue.FindStop(stop_name) == nullptr) {
            builder.Key("error_message").Value("not found").EndDict();
        } else {
            builder.Key("buses").StartArray().EndArray().EndDict();
        }
    } else {
        builder.Key("buses").StartArray();
        for (const auto& bus : buses) {
            builder.Value(bus);
        }
        builder.EndArray().EndDict();
    }

    return builder.Build();
}

json::Node JsonReader::ProcessRouteRequest(const TransportCatalogue& catalogue,  const json::Dict& req,const transport::TransportRouter& router) {
    (void)catalogue;

    auto route_info = router.BuildRoute(req.at("from").AsString(),  req.at("to").AsString());
    
    json::Builder builder;
    builder.StartDict().Key("request_id").Value(req.at("id").AsInt());
    
    if (!route_info) {
        builder.Key("error_message").Value("not found");
    } else {
        builder.Key("total_time").Value(route_info->total_time)
               .Key("items").StartArray();
        
        for (const auto& item : route_info->items) {
            builder.StartDict()
                   .Key("type").Value(item.type);
            
            if (item.type == "Wait") {
                builder.Key("stop_name").Value(item.stop_name)
                       .Key("time").Value(item.time);
            } else { 
                builder.Key("bus").Value(item.bus_name)
                       .Key("span_count").Value(item.span_count)
                       .Key("time").Value(item.time);
            }
            builder.EndDict();
        }
        builder.EndArray();
    }
    builder.EndDict();
    return builder.Build();
}