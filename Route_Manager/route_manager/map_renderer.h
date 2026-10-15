#pragma once
#include "geo.h"
#include "svg.h"
#include "transport_catalogue.h"
#include <vector>
#include <array>
#include <optional>
#include <cmath>
#include <algorithm>
#include <sstream>

namespace map_renderer {

struct RenderSettings {
    double width = 0.0;
    double height = 0.0;
    double padding = 0.0;
    double line_width = 0.0;
    double stop_radius = 0.0;
    double underlayer_width = 0.0;

    int bus_label_font_size = 0;
    int stop_label_font_size = 0;

    std::array<double, 2> bus_label_offset{};
    std::array<double, 2> stop_label_offset{};

    svg::Color underlayer_color;
    std::vector<svg::Color> color_palette;
};

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) {
        using namespace std;
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = minmax_element(points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = minmax_element(points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        optional<double> width_zoom;
        if (abs(max_lon - min_lon_) > 1e-9) {
            width_zoom = (max_width - 2 * padding) / abs(max_lon - min_lon_); 
        }

        optional<double> height_zoom;
        if (abs(max_lat_ - min_lat) > 1e-9) {
            height_zoom = (max_height - 2 * padding) / abs(max_lat_ - min_lat); 
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        } else {
            zoom_coeff_ = 0.0;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_ = 0;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    void SetSettings(RenderSettings settings);

    svg::Document RenderMap(const TransportCatalogue& catalogue) const;

private:
    void RenderBusLines(const TransportCatalogue& catalogue,
                        const SphereProjector& projector,
                        svg::Document& doc) const;

    void RenderBusLabels(const TransportCatalogue& catalogue,
                         const SphereProjector& projector,
                         svg::Document& doc) const;

    void RenderStopSymbols(const TransportCatalogue& catalogue,
                           const SphereProjector& projector,
                           svg::Document& doc) const;

    void RenderStopLabels(const TransportCatalogue& catalogue,
                          const SphereProjector& projector,
                          svg::Document& doc) const;

    RenderSettings settings_;
};

} // namespace map_renderer