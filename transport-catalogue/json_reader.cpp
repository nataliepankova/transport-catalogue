#include "json_reader.h"

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

void JsonReader::ApplyBaseRequests(const Array& request_array, transport_catalogue::TransportCatalogue& catalogue) const {
	for (const Node& request_node : request_array) {
		Dict request = request_node.AsMap();
		if (request.at("type"s).AsString() == "Stop"s) {
			catalogue.AddStop(request.at("name"s).AsString(), { request.at("latitude"s).AsDouble(),  request.at("longitude"s).AsDouble() });
		}
	}
	for (const Node& request_node : request_array) {
		Dict request = request_node.AsMap();
		if (request.at("type"s).AsString() == "Stop"s) {
			for (const auto& dst : detail::ParseDistanceToStop(request.at("road_distances"s))) {
				catalogue.SetStopDistances(request.at("name"s).AsString(), dst.stop_name, dst.distance);
			}
		}
	}
	for (const Node& request_node : request_array) {
		Dict request = request_node.AsMap();
		if (request.at("type"s).AsString() == "Bus"s) {
			bool is_roundtrip = request.at("is_roundtrip"s).AsBool();
			catalogue.AddBus(request.at("name"s).AsString(), detail::ParseRoute(request.at("stops"s), is_roundtrip), is_roundtrip);
		}
	}
	
}

Node JsonReader::PrepareBusStat(const Dict& request, transport_catalogue::TransportCatalogue& catalogue) const {
	std::string bus_name = request.at("name"s).AsString();
	Dict bus_stat{ {"request_id", request.at("id"s).AsInt()} };
	if (catalogue.FindBus(bus_name) == nullptr) {

		bus_stat["error_message"] = Node("not found"s);
	}
	else {
		auto bus_info = catalogue.GetBusInfo(bus_name);
		bus_stat["curvature"] = Node(bus_info.curvature);
		bus_stat["route_length"] = Node(bus_info.route_length);
		bus_stat["stop_count"] = Node(static_cast<int>(bus_info.all_stops_count));
		bus_stat["unique_stop_count"] = Node(static_cast<int>(bus_info.unique_stops_count));
	}

	return Node(bus_stat);

}

Node JsonReader::PrepareStopStat(const Dict& request, transport_catalogue::TransportCatalogue& catalogue) const {
	std::string stop_name = request.at("name"s).AsString();
	Dict stop_stat{ {"request_id", request.at("id"s).AsInt()} };
	if (catalogue.FindStop(stop_name) == nullptr) {

		stop_stat["error_message"] = Node("not found"s);
	}
	else {
		auto stop_info = catalogue.GetStopInfo(stop_name);
		if (stop_info.empty()) {
			stop_stat["buses"] = Node(Array{});
		}
		else {
			Array buses;
			buses.reserve(stop_info.size());
			for (const auto& bus : stop_info) {
				buses.emplace_back(bus->name);
			}
			stop_stat["buses"] = Node(buses);
		}

	}

	return Node(stop_stat);
}

Node JsonReader::PrepareMap(const Dict& request, std::set<const Bus*, BusSetCmp>& buses, renderer::MapRenderer& renderer) const {
	std::ostringstream out;
	renderer.RenderMap(buses, out);
	Dict map_data{ {"request_id", request.at("id"s).AsInt()}, {"map", out.str()} };
	return Node(map_data);
}

void JsonReader::ApplyStatRequests(const Array& request_array, transport_catalogue::TransportCatalogue& catalogue, renderer::MapRenderer& renderer) const {
	Array result;
	for (const Node& request_node : request_array) {
		Dict request = request_node.AsMap();
		if (request.at("type"s).AsString() == "Bus"s) {
			result.emplace_back(PrepareBusStat(request, catalogue));
		}
		if (request.at("type"s).AsString() == "Stop"s) {
			result.emplace_back(PrepareStopStat(request, catalogue));
		}
		if (request.at("type"s).AsString() == "Map"s) {
			auto buses = catalogue.GetBuses();
			result.emplace_back(PrepareMap(request, buses, renderer));
		}
	}
	Print(Document{ Node{result} }, std::cout);
}

void JsonReader::ApplyCommands(transport_catalogue::TransportCatalogue& catalogue, renderer::MapRenderer& renderer) const {
	Dict requests = json_doc_.GetRoot().AsMap();
	if (requests.count("base_requests"s)) {
		Array base_requests = requests.at("base_requests"s).AsArray();
		ApplyBaseRequests(base_requests, catalogue);
	}
	if (requests.count("render_settings"s)) {
		Dict render_settings = requests.at("render_settings"s).AsMap();
		ApplyRenderSettings(render_settings, renderer);
	}
	if (requests.count("stat_requests"s)) {
		Array stat_requests = requests.at("stat_requests"s).AsArray();
		if (!stat_requests.empty()) {
			ApplyStatRequests(stat_requests, catalogue, renderer);
		}
	}
}

void JsonReader::ApplyRenderSettings(const Dict& render_settings, renderer::MapRenderer& renderer) const {
	renderer.width_ = render_settings.at("width"s).AsDouble();
	renderer.height_ = render_settings.at("height"s).AsDouble();
	renderer.padding_ = render_settings.at("padding"s).AsDouble();
	renderer.stop_radius_ = render_settings.at("stop_radius"s).AsDouble();
	renderer.line_width_ = render_settings.at("line_width"s).AsDouble();
	renderer.bus_label_font_size_ = render_settings.at("bus_label_font_size"s).AsInt();
	renderer.bus_label_offset_= renderer.CreatePoint(render_settings.at("bus_label_offset"s).AsArray()[0].AsDouble(), render_settings.at("bus_label_offset"s).AsArray()[1].AsDouble());
	renderer.stop_label_font_size_ = render_settings.at("stop_label_font_size"s).AsInt();
	renderer.stop_label_offset_ = renderer.CreatePoint(render_settings.at("stop_label_offset"s).AsArray()[0].AsDouble(), render_settings.at("stop_label_offset"s).AsArray()[1].AsDouble());
	if (render_settings.at("underlayer_color"s).IsString()) {
		renderer.underlayer_color_ = render_settings.at("underlayer_color"s).AsString();
	}
	else if (render_settings.at("underlayer_color"s).IsArray()) {
		Array shades = render_settings.at("underlayer_color"s).AsArray();
		if (shades.size() == 3) {
			renderer.underlayer_color_ = renderer.CreateRgbColor(shades[0].AsInt(), shades[1].AsInt(), shades[2].AsInt());
		}
		else if (shades.size() == 4) {
			renderer.underlayer_color_ = renderer.CreateRgbaColor(shades[0].AsInt(), shades[1].AsInt(), shades[2].AsInt(), shades[3].AsDouble());
		}
	}
	renderer.underlayer_width_ = render_settings.at("underlayer_width"s).AsDouble();
	for (const Node& color : render_settings.at("color_palette"s).AsArray()) {
		if (color.IsString()) {
			renderer.color_palette_.push_back(color.AsString());
		}
		else if (color.IsArray()) {
			Array shades = color.AsArray();
			if (shades.size() == 3) {
				renderer.color_palette_.push_back(renderer.CreateRgbColor(shades[0].AsInt(), shades[1].AsInt(), shades[2].AsInt()));
			}
			else if (shades.size() == 4) {
				renderer.color_palette_.push_back(renderer.CreateRgbaColor(shades[0].AsInt(), shades[1].AsInt(), shades[2].AsInt(), shades[3].AsDouble()));
			}
		}
	}
}