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

std::vector<MapRenderer::StopPoint> MapRenderer::PrepareStopPointsForRoute(const std::vector<Stop*> bus_route, const SphereProjector proj) const {
	std::vector<StopPoint> result;
	for (const Stop* stop : bus_route) {
		result.emplace_back(StopPoint{ stop->name, proj(geo::Coordinates{ stop->coordinates.lat, stop->coordinates.lng }) });
	}
	return result;
}

std::set<MapRenderer::StopPoint, MapRenderer::StopPointCmp> MapRenderer::PrepareSortedUniqueStopPoints(std::set<const Bus*, BusSetCmp>& buses, const SphereProjector proj) const {
	std::set<StopPoint, StopPointCmp> result;
	for (const Bus* bus : buses) {
		if (!bus->route.empty()) {
			// запишем плоскостные точки маршрута
			std::vector<StopPoint> stops_points = PrepareStopPointsForRoute(bus->route, proj);

			// запишем уникальные остановки в отсортированном виде
			for (const auto& stop_point : stops_points) {
				result.emplace(StopPoint{ stop_point });
			}
		}
	}
	return result;
}

Color MapRenderer::GetRouteColor(size_t bus_index) const {
	size_t color_palette_index = bus_index < color_palette_.size() ? bus_index : bus_index % color_palette_.size();
	return color_palette_[color_palette_index];
}

std::vector<Polyline> MapRenderer::RenderBusRoutes(std::set<const Bus*, BusSetCmp>& buses, const SphereProjector proj) const {
	size_t bus_index = 0;
	std::vector<Polyline> bus_routes;

	for (const Bus* bus : buses) {
		if (!bus->route.empty()) {
			// запишем плоскостные точки маршрута
			std::vector<StopPoint> stops_points = PrepareStopPointsForRoute(bus->route, proj);

			// определим цвет маршрута и обновим индекс палитры
			Color route_color = GetRouteColor(bus_index);

			bus_routes.emplace_back(RenderRoute(stops_points, route_color));
			++bus_index;
		}
	}
	return bus_routes;

}

std::vector<Text> MapRenderer::RenderBusCaptions(std::set<const Bus*, BusSetCmp>& buses, const SphereProjector proj) const {
	std::vector<Text> bus_captions;
	size_t bus_index = 0;
	for (const Bus* bus : buses) {
		if (!bus->route.empty()) {
			// запишем плоскостные точки маршрута
			std::vector<StopPoint> stops_points = PrepareStopPointsForRoute(bus->route, proj);

			// определим цвет маршрута и обновим индекс палитры
			Color route_color = GetRouteColor(bus_index);

			// добавляем названия автобусов
			bus_captions.emplace_back(RenderBusNameUnderlayer(stops_points[0].point, bus->name));
			bus_captions.emplace_back(RenderBusNameText(stops_points[0].point, bus->name, route_color));
			if (!bus->is_roundtrip) {
				Point second_end_stop_point = stops_points[stops_points.size() / 2].point;
				if (second_end_stop_point != stops_points[0].point) {
					bus_captions.emplace_back(RenderBusNameUnderlayer(second_end_stop_point, bus->name));
					bus_captions.emplace_back(RenderBusNameText(second_end_stop_point, bus->name, route_color));
				}
			}
			++bus_index;
		}
	}
	return bus_captions;
}

std::vector<Circle> MapRenderer::RenderStopCircles(std::set<const Bus*, BusSetCmp>& buses, const SphereProjector proj) const {
	std::vector<Circle> result;
	std::set<StopPoint, StopPointCmp> unique_stops = PrepareSortedUniqueStopPoints(buses, proj);
	// добавляем кружки остановок
	for (const auto& stop : unique_stops) {
		result.emplace_back(RenderStop(stop.point));
	}
	return result;
}

std::vector<Text> MapRenderer::RenderStopCaptions(std::set<const Bus*, BusSetCmp>& buses, const SphereProjector proj) const {
	std::vector<Text> result;
	std::set<StopPoint, StopPointCmp> unique_stops = PrepareSortedUniqueStopPoints(buses, proj);
	// добавляем надписи для остановок
	for (const auto& stop : unique_stops) {
		result.emplace_back(RenderStopNameUnderlayer(stop));
		result.emplace_back(RenderStopNameText(stop));
	}
	return result;
}


void MapRenderer::RenderMap(std::set<const Bus*, BusSetCmp>& buses, std::ostream& out) const {
	Document map;
	// спроецируем все остановки на плоскость и подготовим масштаб карты
	const SphereProjector proj = ProjectStops(buses);
	// контейнеры для элементов карты
	std::vector<Polyline> bus_routes = RenderBusRoutes(buses, proj);
	std::vector<Text> bus_text = RenderBusCaptions(buses, proj);
	std::vector<Circle> stops = RenderStopCircles(buses, proj);
	std::vector<Text> stop_text = RenderStopCaptions(buses, proj);

	// добавляем элементы на карту
	for (const Polyline& route : bus_routes) {
		map.Add(route);
	}

	for (const Text& elem : bus_text) {
		map.Add(elem);
	}

	// добавляем кружки остановок
	for (const Circle& stop : stops) {
		map.Add(stop);
	}

	// добавляем надписи для остановок
	for (const Text& elem : stop_text) {
		map.Add(elem);
	}
	
	map.Render(out);
}
