#include "map_renderer.h"
#include <algorithm>
#include <iostream>

using namespace std;

namespace map_renderer {

void MapRenderer::SetSettings(RenderSettings settings) {
    settings_ = std::move(settings);
}

svg::Document MapRenderer::RenderMap(const TransportCatalogue& catalogue) const {
    svg::Document doc;

    vector<geo::Coordinates> all_coords;
    for (const auto& bus : catalogue.GetBuses()) {
        for (const auto& stop_name : bus.stops) {
            if (auto stop_ptr = catalogue.FindStop(stop_name)) {
                all_coords.push_back(stop_ptr->coordinates);
            }
        }
    }

    SphereProjector projector(all_coords.begin(), all_coords.end(),
                              settings_.width, settings_.height, settings_.padding);

    RenderBusLines(catalogue, projector, doc);
    RenderBusLabels(catalogue, projector, doc);
    RenderStopSymbols(catalogue, projector, doc);
    RenderStopLabels(catalogue, projector, doc);

    return doc;
}

void MapRenderer::RenderBusLines(const TransportCatalogue& catalogue, const SphereProjector& projector, svg::Document& doc) const {
    const auto& buses = catalogue.GetBuses();

    std::vector<const domain::Bus*> sorted_buses;
    for (const auto& bus : buses) {
        if (!bus.stops.empty()) {
            sorted_buses.push_back(&bus);
        }
    }

    std::sort(sorted_buses.begin(), sorted_buses.end(),
              [](const domain::Bus* lhs, const domain::Bus* rhs) {
                  return lhs->name < rhs->name;
              });

    size_t color_index = 0;

    for (const auto* bus : sorted_buses) {
        if (bus->stops.empty()) continue;

        svg::Polyline line;
        line.SetFillColor("none")
            .SetStrokeColor(settings_.color_palette[color_index])
            .SetStrokeWidth(settings_.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        for (const auto& stop_name : bus->stops) {
            if (auto stop_ptr = catalogue.FindStop(stop_name)) {
                line.AddPoint(projector(stop_ptr->coordinates));
            }
        }

        if (!bus->is_roundtrip) {
            for (auto it = bus->stops.rbegin() + 1; it != bus->stops.rend(); ++it) {
                if (auto stop_ptr = catalogue.FindStop(*it)) {
                    line.AddPoint(projector(stop_ptr->coordinates));
                }
            }
        }

        doc.Add(line);
        color_index = (color_index + 1) % settings_.color_palette.size();
    }
}

void MapRenderer::RenderBusLabels(const TransportCatalogue& catalogue, const SphereProjector& projector, svg::Document& doc) const {
    const auto& buses = catalogue.GetBuses();

    std::vector<const domain::Bus*> sorted_buses;
    for (const auto& bus : buses) {
        if (!bus.stops.empty()) {
            sorted_buses.push_back(&bus);
        }
    }

    std::sort(sorted_buses.begin(), sorted_buses.end(),
              [](const domain::Bus* lhs, const domain::Bus* rhs) {
                  return lhs->name < rhs->name;
              });

    size_t color_index = 0;

    for (const auto* bus : sorted_buses) {
        if (bus->stops.empty()) continue;

        const auto& color = settings_.color_palette[color_index];

        domain::Stop* first_stop = catalogue.FindStop(bus->stops.front());
        domain::Stop* last_stop = catalogue.FindStop(bus->stops.back());

        if (first_stop) {
            svg::Point pos = projector(first_stop->coordinates);

        svg::Text underlayer;
        underlayer.SetPosition(pos)
            .SetOffset({settings_.bus_label_offset[0], settings_.bus_label_offset[1]})
            .SetFontSize(settings_.bus_label_font_size)
            .SetFontFamily("Verdana")
            .SetFontWeight("bold")
            .SetData(bus->name)
            .SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        svg::Text text;
        text.SetPosition(pos)
            .SetOffset({settings_.bus_label_offset[0], settings_.bus_label_offset[1]})
            .SetFontSize(settings_.bus_label_font_size)
            .SetFontFamily("Verdana")
            .SetFontWeight("bold")
            .SetData(bus->name)
            .SetFillColor(color);

        doc.Add(underlayer);
        doc.Add(text);
        }

        if (!bus->is_roundtrip && last_stop && last_stop != first_stop) {
            svg::Point pos = projector(last_stop->coordinates);

        svg::Text underlayer;
        underlayer.SetPosition(pos)
            .SetOffset({settings_.bus_label_offset[0], settings_.bus_label_offset[1]})
            .SetFontSize(settings_.bus_label_font_size)
            .SetFontFamily("Verdana")
            .SetFontWeight("bold")
            .SetData(bus->name)
            .SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        svg::Text text;
        text.SetPosition(pos)
            .SetOffset({settings_.bus_label_offset[0], settings_.bus_label_offset[1]})
            .SetFontSize(settings_.bus_label_font_size)
            .SetFontFamily("Verdana")
            .SetFontWeight("bold")
            .SetData(bus->name)
            .SetFillColor(color);

        doc.Add(underlayer);
        doc.Add(text);
        }

        color_index = (color_index + 1) % settings_.color_palette.size();
    }
}

void MapRenderer::RenderStopSymbols(const TransportCatalogue& catalogue, const SphereProjector& projector, svg::Document& doc) const {
    const auto& stops = catalogue.GetStops();

    std::vector<const domain::Stop*> sorted_stops;
    for (const auto& stop : stops) {
        if (!catalogue.GetBusesForStop(stop.name).empty()) {
            sorted_stops.push_back(&stop);
        }
    }

    std::sort(sorted_stops.begin(), sorted_stops.end(),
              [](const domain::Stop* lhs, const domain::Stop* rhs) {
                  return lhs->name < rhs->name;
              });

    for (const auto* stop : sorted_stops) {
        svg::Point pos = projector(stop->coordinates);

        svg::Circle circle;
        circle.SetCenter(pos)
              .SetRadius(settings_.stop_radius)
              .SetFillColor("white");

        doc.Add(circle);
    }
}

void MapRenderer::RenderStopLabels(const TransportCatalogue& catalogue, const SphereProjector& projector, svg::Document& doc) const {
    const auto& stops = catalogue.GetStops();
    std::vector<const domain::Stop*> sorted_stops;

    for (const auto& stop : stops) {
        if (!catalogue.GetBusesForStop(stop.name).empty()) {
            sorted_stops.push_back(&stop);
        }
    }

    std::sort(sorted_stops.begin(), sorted_stops.end(),
              [](const domain::Stop* lhs, const domain::Stop* rhs) {
                  return lhs->name < rhs->name;
              });

    for (const auto* stop : sorted_stops) {
        svg::Point pos = projector(stop->coordinates);

        svg::Text underlayer_text;
        underlayer_text.SetPosition(pos)
               .SetOffset({settings_.stop_label_offset[0], settings_.stop_label_offset[1]})
               .SetFontSize(settings_.stop_label_font_size)
               .SetFontFamily("Verdana")
               .SetData(stop->name)
               .SetFillColor(settings_.underlayer_color)
               .SetStrokeColor(settings_.underlayer_color)
               .SetStrokeWidth(settings_.underlayer_width)
               .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
               .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        svg::Text label_text;
        label_text.SetPosition(pos)
               .SetOffset({settings_.stop_label_offset[0],     settings_.stop_label_offset[1]})
               .SetFontSize(settings_.stop_label_font_size)
               .SetFontFamily("Verdana")
               .SetData(stop->name)
               .SetFillColor("black");

        doc.Add(underlayer_text);
        doc.Add(label_text);
    }
}

} // namespace map_renderer