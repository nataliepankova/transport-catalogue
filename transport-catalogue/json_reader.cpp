#include "json_reader.h"
#include "json_builder.h"

#include <sstream>

using namespace std::literals;
using namespace json_reader;

std::vector<DistanceToStop> detail::ParseDistanceToStop(const Node& stop_info) {
	std::vector<DistanceToStop> result;
	for (const auto& [key, val] : stop_info.AsMap()) {
		result.emplace_back(DistanceToStop { key, val.AsInt()});
	}
	return result;
}


std::vector<std::string_view> detail::ParseRoute(const Node& route, bool is_roundtrip) {
	std::vector<std::string_view> stops;

	stops.reserve(route.AsArray().size());
	for (const auto& elem : route.AsArray()) {
		stops.emplace_back(elem.AsString());
	}
	std::vector<std::string_view> results(stops.begin(), stops.end());
	if (is_roundtrip) {
		return results;
	}
	results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

	return results;
}

void JsonReader::AddStopsToCatalogue(const Array& request_array, transport_catalogue::TransportCatalogue& catalogue) const {
	for (const Node& request_node : request_array) {
		Dict request = request_node.AsMap();
		if (request.at("type"s).AsString() == "Stop"s) {
			catalogue.AddStop(request.at("name"s).AsString(), { request.at("latitude"s).AsDouble(),  request.at("longitude"s).AsDouble() });
		}
	}
}

void JsonReader::SetStopDistancesInCatalogue(const Array& request_array, transport_catalogue::TransportCatalogue& catalogue) const {
	for (const Node& request_node : request_array) {
		Dict request = request_node.AsMap();
		if (request.at("type"s).AsString() == "Stop"s) {
			for (const auto& dst : detail::ParseDistanceToStop(request.at("road_distances"s))) {
				catalogue.SetStopDistances(request.at("name"s).AsString(), dst.stop_name, dst.distance);
			}
		}
	}
}

void JsonReader::AddBusesToCatalogue(const Array& request_array, transport_catalogue::TransportCatalogue& catalogue) const {
	for (const Node& request_node : request_array) {
		Dict request = request_node.AsMap();
		if (request.at("type"s).AsString() == "Bus"s) {
			bool is_roundtrip = request.at("is_roundtrip"s).AsBool();
			catalogue.AddBus(request.at("name"s).AsString(), detail::ParseRoute(request.at("stops"s), is_roundtrip), is_roundtrip);
		}
	}
}

void JsonReader::ApplyBaseRequests(transport_catalogue::TransportCatalogue& catalogue) const {
	Dict requests = json_doc_.GetRoot().AsMap();
	if (!requests.count("base_requests"s)) {
		return;
	}
	Array base_requests = requests.at("base_requests"s).AsArray();
	AddStopsToCatalogue(base_requests, catalogue);
	SetStopDistancesInCatalogue(base_requests, catalogue);
	AddBusesToCatalogue(base_requests, catalogue);
}

Node JsonReader::PrepareBusStat(const Dict& request, transport_catalogue::TransportCatalogue& catalogue) const {
	json::Builder bus_stat{};
	bus_stat.StartDict().Key("request_id"s).Value(request.at("id"s).AsInt());
	std::string bus_name = request.at("name"s).AsString();
	if (catalogue.FindBus(bus_name) == nullptr) {

		bus_stat.Key("error_message"s).Value("not found"s);
	}
	else {
		auto bus_info = catalogue.GetBusInfo(bus_name);
		bus_stat.Key("curvature"s).Value(bus_info.curvature)
				.Key("route_length"s).Value(bus_info.route_length)
				.Key("stop_count"s).Value(static_cast<int>(bus_info.all_stops_count))
				.Key("unique_stop_count"s).Value(static_cast<int>(bus_info.unique_stops_count));
	}

	return bus_stat.EndDict().Build();

}

Node JsonReader::PrepareStopStat(const Dict& request, transport_catalogue::TransportCatalogue& catalogue) const {
	json::Builder stop_stat{};
	stop_stat.StartDict().Key("request_id"s).Value(request.at("id"s).AsInt());
	std::string stop_name = request.at("name"s).AsString();
	if (catalogue.FindStop(stop_name) == nullptr) {
		stop_stat.Key("error_message"s).Value("not found"s);
	}
	else {
		auto stop_info = catalogue.GetStopInfo(stop_name);
		if (stop_info.empty()) {
			stop_stat.Key("buses"s).StartArray().EndArray();
		}
		else {
			stop_stat.Key("buses"s).StartArray();
			for (const auto& bus : stop_info) {
				stop_stat.Value(bus->name);
			}
			stop_stat.EndArray();
		}

	}

	return stop_stat.EndDict().Build();
}

Node JsonReader::PrepareMap(const Dict& request, std::set<const Bus*, BusSetCmp>& buses, renderer::MapRenderer& renderer) const {
	std::ostringstream out;
	renderer.RenderMap(buses, out);
	json::Builder map_data{};
	return map_data.StartDict().Key("request_id"s).Value(request.at("id"s).AsInt())
		.Key("map"s).Value(out.str())
		.EndDict().Build();
}

Node JsonReader::PrepareRouteStat(const Dict& request, transport_router::TransportRouter& router) const {
	std::string_view stop_from = request.at("from"s).AsString();
	std::string_view stop_to = request.at("to"s).AsString();
	std::optional<transport_router::TranspRouteInfo> route_info = router.MakeRoute(stop_from, stop_to);
	json::Builder route_json{};
	route_json.StartDict().Key("request_id"s).Value(request.at("id"s).AsInt());
	if (!route_info) {
		return route_json.Key("error_message"s).Value("not found"s).EndDict().Build();
	}
	route_json.Key("total_time"s).Value(route_info->total_time)
				.Key("items"s).StartArray() ;
	for (const auto& item : route_info->items) {
		if (item.type == EdgeType::WAIT) {
			route_json.StartDict().Key("type"s).Value("Wait"s)
				.Key("stop_name"s).Value(item.name)
				.Key("time"s).Value(item.time)
				.EndDict();
			continue;
		}
		if (item.type == EdgeType::BUS) {
			route_json.StartDict().Key("type"s).Value("Bus"s)
				.Key("bus"s).Value(std::string(item.name))
				.Key("span_count"s).Value(item.span_count.value())
				.Key("time"s).Value(item.time)
				.EndDict();
			continue;
		}
	}
	return route_json.EndArray().EndDict().Build();

}

void JsonReader::ApplyStatRequests(transport_catalogue::TransportCatalogue& catalogue, renderer::MapRenderer& renderer, transport_router::TransportRouter& router) const {
	Dict requests = json_doc_.GetRoot().AsMap();
	if (!requests.count("stat_requests"s)) {
		return;
	}
	Array stat_requests = requests.at("stat_requests"s).AsArray();
	if (stat_requests.empty()) {
		return;
	}
	json::Builder result{};
	result.StartArray();
	for (const Node& request_node : stat_requests) {
		Dict request = request_node.AsMap();
		if (request.at("type"s).AsString() == "Bus"s) {
			result.Value(PrepareBusStat(request, catalogue).AsMap());
		}
		if (request.at("type"s).AsString() == "Stop"s) {
			result.Value(PrepareStopStat(request, catalogue).AsMap());
		}
		if (request.at("type"s).AsString() == "Map"s) {
			auto buses = catalogue.GetBuses();
			result.Value(PrepareMap(request, buses, renderer).AsMap());
		}
		if (request.at("type"s).AsString() == "Route"s) {
			result.Value(PrepareRouteStat(request, router).AsMap());
		}
	}
	Print(Document{ result.EndArray().Build()}, std::cout);
}

svg::Color JsonReader::CreateColorFromArray(const Array& shades, renderer::MapRenderer& renderer) const {
	if (shades.size() == 3) {
		return renderer.CreateRgbColor(shades[0].AsInt(), shades[1].AsInt(), shades[2].AsInt());
	}
	else if (shades.size() == 4) {
		return renderer.CreateRgbaColor(shades[0].AsInt(), shades[1].AsInt(), shades[2].AsInt(), shades[3].AsDouble());
	}
	else {
		return svg::NoneColor;
	}
}

void JsonReader::ApplyRenderSettings(renderer::MapRenderer& renderer) const {
	Dict requests = json_doc_.GetRoot().AsMap();
	if (!requests.count("render_settings"s)) {
		return;
	}
	Dict render_settings = requests.at("render_settings"s).AsMap();
	renderer.width_ = render_settings.at("width"s).AsDouble();
	renderer.height_ = render_settings.at("height"s).AsDouble();
	renderer.padding_ = render_settings.at("padding"s).AsDouble();
	renderer.stop_radius_ = render_settings.at("stop_radius"s).AsDouble();
	renderer.line_width_ = render_settings.at("line_width"s).AsDouble();
	renderer.bus_label_font_size_ = render_settings.at("bus_label_font_size"s).AsInt();
	renderer.bus_label_offset_ = renderer.CreatePoint(render_settings.at("bus_label_offset"s).AsArray()[0].AsDouble(), render_settings.at("bus_label_offset"s).AsArray()[1].AsDouble());
	renderer.stop_label_font_size_ = render_settings.at("stop_label_font_size"s).AsInt();
	renderer.stop_label_offset_ = renderer.CreatePoint(render_settings.at("stop_label_offset"s).AsArray()[0].AsDouble(), render_settings.at("stop_label_offset"s).AsArray()[1].AsDouble());
	if (render_settings.at("underlayer_color"s).IsString()) {
		renderer.underlayer_color_ = render_settings.at("underlayer_color"s).AsString();
	}
	else if (render_settings.at("underlayer_color"s).IsArray()) {
		renderer.underlayer_color_ = CreateColorFromArray(render_settings.at("underlayer_color"s).AsArray(), renderer);
	}
	renderer.underlayer_width_ = render_settings.at("underlayer_width"s).AsDouble();
	for (const Node& color : render_settings.at("color_palette"s).AsArray()) {
		if (color.IsString()) {
			renderer.color_palette_.push_back(color.AsString());
		}
		else if (color.IsArray()) {
			renderer.color_palette_.push_back(CreateColorFromArray(color.AsArray(), renderer));
		}
	}
}

transport_router::TranspRouteParams JsonReader::GetRoutingSettings() const {
	Dict requests = json_doc_.GetRoot().AsMap();
	transport_router::TranspRouteParams params;
	if (!requests.count("routing_settings"s)) {
		return params;
	}
	Dict routing_settings = requests.at("routing_settings"s).AsMap();
	params.bus_wait_time = routing_settings.at("bus_wait_time"s).AsInt();
	params.bus_velocity = routing_settings.at("bus_velocity"s).AsDouble();
	return params;
}
