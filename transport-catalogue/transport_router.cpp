#include "transport_router.h"

#include <algorithm>

using namespace std::literals;
using namespace transport_router;

TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& transport_catalogue, const TranspRouteParams& params)
	:transport_catalogue_(transport_catalogue),
	graph_(Graph{ transport_catalogue_.GetStops().size() * 2 }),
	router_(nullptr),
	params_(params)
{
	MakeGraph();
	router_ = std::make_unique<Router>(graph_);
}

double TransportRouter::CalculateTime(double distance, double velocity) {
	double distance_in_km = distance / METERS_IN_KM * 1.0;
	double time_in_hour = distance_in_km / velocity;
	return time_in_hour * MINUTES_IN_HOURS;
}

void TransportRouter::AddStopsToGraph() {
	size_t curr_vertex_id = 0;
	// draw stops
	for (const auto& stop : transport_catalogue_.GetStops()) {
		// add pairs of vertices for stops
		stops_to_vertex_ids_[stop.name] = { curr_vertex_id, curr_vertex_id + 1 };
		graph_.AddEdge({ stops_to_vertex_ids_.at(stop.name).stop_wait_id, stops_to_vertex_ids_.at(stop.name).stop_go_id, static_cast<double>(params_.bus_wait_time), EdgeType::WAIT, stop.name, 0 });

		curr_vertex_id += 2;
	}
}

void TransportRouter::MakeGraph() {
	AddStopsToGraph();
	std::set<const Bus*, BusSetCmp> buses = transport_catalogue_.GetBuses();
	// add edges for bus routes
	for (const auto& bus : buses) {
		AddBusRoutesToGraph(bus->route.begin(), bus->route.end(), bus->name);
		if (!bus->is_roundtrip) {
			AddBusRoutesToGraph(bus->route.rbegin(), bus->route.rend(), bus->name);
		}
	}
}

std::optional<TranspRouteInfo> TransportRouter::MakeRoute(std::string_view stop_from, std::string_view stop_to) {
	TranspRouteInfo result;
	if (stop_from == stop_to) {
		return result;
	}
	auto route_info =  router_->BuildRoute(stops_to_vertex_ids_.at(std::string(stop_from)).stop_wait_id, stops_to_vertex_ids_.at(std::string(stop_to)).stop_wait_id);
	if (!route_info) {
		return std::nullopt;
	}
	result.total_time = route_info->weight;

	// parse edges of optimal route
	for (const auto& edge : route_info->edges) {
		const Edge<double>& curr_edge_data = graph_.GetEdge(edge);
		// if we get no bus name - push wait item
		if (curr_edge_data.type == EdgeType::WAIT) {
			result.items.emplace_back(TranspRouteInfo::RouteItemInfo{ EdgeType::WAIT, curr_edge_data.entity_name, 0, static_cast<double>(params_.bus_wait_time) });

		} else if (curr_edge_data.type == EdgeType::BUS) {
			result.items.emplace_back(TranspRouteInfo::RouteItemInfo{ EdgeType::BUS, curr_edge_data.entity_name, curr_edge_data.span_count, curr_edge_data.weight });

		}
	}

	return result;
}
