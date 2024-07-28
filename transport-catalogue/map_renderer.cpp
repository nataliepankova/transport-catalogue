#include "map_renderer.h"
#include "request_handler.h"

using namespace svg;
using namespace renderer;
using namespace std::literals;

inline const double EPSILON = 1e-6;
bool renderer::IsZero(double value) {
	return std::abs(value) < EPSILON;
}

Point MapRenderer::CreatePoint(double dx, double dy) const {
	return Point{ dx, dy };
}

Color MapRenderer::CreateRgbColor(int red_shade, int green_shade, int blue_shade) const {
	return Rgb{ static_cast<uint8_t>(red_shade), static_cast<uint8_t>(green_shade), static_cast<uint8_t>(blue_shade) };
}

Color MapRenderer::CreateRgbaColor(int red_shade, int green_shade, int blue_shade, double opacity) const {
	return Rgba{ static_cast<uint8_t>(red_shade), static_cast<uint8_t>(green_shade), static_cast<uint8_t>(blue_shade), opacity };
}



Polyline MapRenderer::RenderRoute(std::vector<StopPoint> stops_points, Color route_color) const {
	Polyline route;
	for (const auto& stop_point : stops_points) {
		route.AddPoint(stop_point.point);
	}
	return route.SetStrokeColor(route_color).SetFillColor(NoneColor).SetStrokeWidth(line_width_).SetStrokeLineCap(StrokeLineCap::ROUND).SetStrokeLineJoin(StrokeLineJoin::ROUND);
}

Text MapRenderer::RenderCommonBusTextProps(const Point& point, std::string bus_name) const {
	return Text().SetPosition(point)
		.SetOffset(bus_label_offset_)
		.SetFontSize(bus_label_font_size_)
		.SetFontFamily("Verdana"s)
		.SetFontWeight("bold"s)
		.SetData(bus_name);
}

Text MapRenderer::RenderBusNameUnderlayer(const Point& point, std::string bus_name) const {
	 return RenderCommonBusTextProps(point, bus_name)
		.SetFillColor(underlayer_color_)
		.SetStrokeColor(underlayer_color_)
		.SetStrokeWidth(underlayer_width_)
		.SetStrokeLineCap(StrokeLineCap::ROUND)
		.SetStrokeLineJoin(StrokeLineJoin::ROUND);
}

Text MapRenderer::RenderBusNameText(const Point& point, std::string bus_name, Color route_color) const {
	return RenderCommonBusTextProps(point, bus_name).SetFillColor(route_color);
}

Circle MapRenderer::RenderStop(const svg::Point& point) const {
	return Circle().SetCenter(point).SetRadius(stop_radius_).SetFillColor("white"s);
}

Text MapRenderer::RenderCommonStopTextProps(const StopPoint& stop) const {
	return Text().SetPosition(stop.point)
		.SetOffset(stop_label_offset_)
		.SetFontSize(stop_label_font_size_)
		.SetFontFamily("Verdana"s)
		.SetData(stop.name);
}

Text MapRenderer::RenderStopNameUnderlayer(const StopPoint& stop) const {
	return RenderCommonStopTextProps(stop)
		.SetFillColor(underlayer_color_)
		.SetStrokeColor(underlayer_color_)
		.SetStrokeWidth(underlayer_width_)
		.SetStrokeLineCap(StrokeLineCap::ROUND)
		.SetStrokeLineJoin(StrokeLineJoin::ROUND);
}

Text MapRenderer::RenderStopNameText(const StopPoint& stop) const {
	return RenderCommonStopTextProps(stop).SetFillColor("black"s);
}

SphereProjector MapRenderer::ProjectStops(std::set<const Bus*, BusSetCmp>& buses) const {
	std::vector<geo::Coordinates> geo_coords;
	// пройдемся по автобусам, что записать все нужные координаты остановок
	for (const Bus* bus : buses) {
		for (const Stop* stop : bus->route) {
			geo_coords.emplace_back(geo::Coordinates{ stop->coordinates.lat, stop->coordinates.lng });
		}
	}
	return SphereProjector{
		geo_coords.begin(), geo_coords.end(), width_, height_, padding_
	};
}


void MapRenderer::RenderMap(std::set<const Bus*, BusSetCmp>& buses, std::ostream& out) const {
	Document map;
	size_t color_palette_index = 0;
	// спроецируем все остановки на плоскость и подготовим масштаб карты
	const SphereProjector proj = ProjectStops(buses);
	// контейнеры для элементов карты
	std::vector<Polyline> bus_routes;
	std::vector<Text> bus_text;

	std::set<StopPoint, StopPointCmp> unique_stops;

	for (const Bus* bus : buses) {
		if (!bus->route.empty()) {
			// запишем плоскостные точки маршрута
			std::vector<StopPoint> stops_points;
			for (const Stop* stop : bus->route) {
				stops_points.emplace_back(StopPoint{ stop->name, proj(geo::Coordinates{ stop->coordinates.lat, stop->coordinates.lng }) });
			}
			
			// определим цвет маршрута и обновим индекс палитры
			Color route_color = color_palette_[color_palette_index];
			color_palette_index = color_palette_index == color_palette_.size() - 1 ? 0 : color_palette_index + 1;
			
			bus_routes.emplace_back(RenderRoute(stops_points, route_color));

			// добавляем названия автобусов
			bus_text.emplace_back(RenderBusNameUnderlayer(stops_points[0].point, bus->name));
			bus_text.emplace_back(RenderBusNameText(stops_points[0].point, bus->name, route_color));
			if (!bus->is_roundtrip) {
				Point second_end_stop_point = stops_points[stops_points.size()/2].point;
				if (second_end_stop_point != stops_points[0].point) {
					bus_text.emplace_back(RenderBusNameUnderlayer(second_end_stop_point, bus->name));
					bus_text.emplace_back(RenderBusNameText(second_end_stop_point, bus->name, route_color));
				}
			}
			
			// запишем уникальные остановки в отсортированном виде
			for (const auto& stop_point : stops_points) {
				unique_stops.emplace(StopPoint{ stop_point });
			}
		}
	}

	// добавляем маршруты на карту
	for (const Polyline& route : bus_routes) {
		map.Add(route);
	}

	for (const Text& elem : bus_text) {
		map.Add(elem);
	}

	// добавляем кружки остановок
	for (const auto& stop : unique_stops) {
		map.Add(RenderStop(stop.point));
	}

	// добавляем надписи для остановок
	for (const auto& stop : unique_stops) {
		map.Add(RenderStopNameUnderlayer(stop));
		map.Add(RenderStopNameText(stop));
	}
	
	map.Render(out);
}
