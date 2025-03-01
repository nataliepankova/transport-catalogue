﻿#pragma once

#include "domain.h"
#include "geo.h"
#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <set>

namespace renderer {

    bool IsZero(double value);

    class SphereProjector {
    public:
        // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
            double max_width, double max_height, double padding)
            : padding_(padding) //
        {
            // Если точки поверхности сферы не заданы, вычислять нечего
            if (points_begin == points_end) {
                return;
            }

            // Находим точки с минимальной и максимальной долготой
            const auto [left_it, right_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;

            // Находим точки с минимальной и максимальной широтой
            const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;

            // Вычисляем коэффициент масштабирования вдоль координаты x
            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            // Вычисляем коэффициент масштабирования вдоль координаты y
            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                // Коэффициенты масштабирования по ширине и высоте ненулевые,
                // берём минимальный из них
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            }
            else if (width_zoom) {
                // Коэффициент масштабирования по ширине ненулевой, используем его
                zoom_coeff_ = *width_zoom;
            }
            else if (height_zoom) {
                // Коэффициент масштабирования по высоте ненулевой, используем его
                zoom_coeff_ = *height_zoom;
            }
        }

        // Проецирует широту и долготу в координаты внутри SVG-изображения
        svg::Point operator()(geo::Coordinates coords) const {
            return {
                (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_
            };
        }

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    struct MapRenderer {
    public:
        void RenderMap(std::set<const Bus*, BusSetCmp>& buses, std::ostream& out) const;
        svg::Point CreatePoint(double dx, double dy) const;
        svg::Color CreateRgbColor(int red_shade, int green_shade, int blue_shade) const;
        svg::Color CreateRgbaColor(int red_shade, int green_shade, int blue_shade, double opacity) const;

        double width_;
        double height_;
        double padding_;
        double line_width_;
        double stop_radius_;
        int bus_label_font_size_;
        svg::Point bus_label_offset_;
        int stop_label_font_size_;
        svg::Point stop_label_offset_;
        svg::Color underlayer_color_;
        double underlayer_width_;
        std::vector<svg::Color> color_palette_;

        struct StopPoint {
            std::string name;
            svg::Point point;
        };
        struct StopPointCmp {
            bool operator()(const StopPoint& lhs, const StopPoint& rhs) const {
                return lhs.name < rhs.name;
            }
        };
    private:
        SphereProjector ProjectStops(std::set<const Bus*, BusSetCmp>& buses) const;
        std::vector<StopPoint> PrepareStopPointsForRoute(const std::vector<Stop*> bus_route, const SphereProjector proj) const;
        std::set<StopPoint, StopPointCmp> PrepareSortedUniqueStopPoints(std::set<const Bus*, BusSetCmp>& buses, const SphereProjector proj) const;
        svg::Color GetRouteColor(size_t bus_index) const;
        svg::Polyline RenderRoute(std::vector<StopPoint> stops_points, svg::Color route_color) const;
        std::vector<svg::Polyline> RenderBusRoutes(std::set<const Bus*, BusSetCmp>& buses, const SphereProjector proj) const;
        std::vector<svg::Text> RenderBusCaptions(std::set<const Bus*, BusSetCmp>& buses, const SphereProjector proj) const;
        svg::Text RenderCommonBusTextProps(const svg::Point& point, std::string bus_name) const;
        svg::Text RenderBusNameUnderlayer(const svg::Point& point, std::string bus_name) const;
        svg::Text RenderBusNameText(const svg::Point& point, std::string bus_name, svg::Color route_color) const;
        svg::Circle RenderStop(const svg::Point& point) const;
        std::vector<svg::Circle> RenderStopCircles(std::set<const Bus*, BusSetCmp>& buses, const SphereProjector proj) const;
        svg::Text RenderCommonStopTextProps(const StopPoint& stop) const;
        svg::Text RenderStopNameUnderlayer(const StopPoint& stop) const;
        svg::Text RenderStopNameText(const StopPoint& stop) const;
        std::vector<svg::Text> RenderStopCaptions(std::set<const Bus*, BusSetCmp>& buses, const SphereProjector proj) const;
    };
}