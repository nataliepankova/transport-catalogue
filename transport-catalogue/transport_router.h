#pragma once

#include <memory>

#include "router.h"
#include "transport_catalogue.h"

using namespace transport_catalogue;
using namespace graph;

namespace transport_router {

	static const int METERS_IN_KM = 1000;
	static const int MINUTES_IN_HOURS = 60;

	struct TranspRouteParams {
		int bus_wait_time;
		double bus_velocity;
	};

	struct StopPairVertex {
		size_t stop_wait_id;
		size_t stop_go_id;
	};

	struct RouteItemInfo {
		EdgeType type;
		std::string name;
		std::optional<int> span_count;
		double time;
	};

	struct TranspRouteInfo {
		double total_time = 0.0;
		std::vector<RouteItemInfo> items;
	};

	using Router = graph::Router<double>;
	using Graph = graph::DirectedWeightedGraph<double>;

	class TransportRouter {
	public:
		TransportRouter() = default;
		TransportRouter(const TransportCatalogue& transp_cat, const std::optional<TranspRouteParams>& params);

		std::optional<TranspRouteInfo> MakeRoute(const std::string& stop_from, const std::string& stop_to);

		double static CalculateTime(double distance, double velocity);

	private:
		const TransportCatalogue& transport_catalogue_;
		Graph graph_;
		std::unique_ptr<Router> router_;
		double bus_wait_time_ = 0;
		double bus_velocity_ = 60;
		std::unordered_map<std::string, StopPairVertex> stops_to_vertex_ids_;

		
		void AddStopsToGraph(); 

		template <typename InputIt>
		void AddBusRoutesToGraph(InputIt begin, InputIt end, std::string bus_name) {
			for (; std::distance(begin, end) != 1; begin++) {
				size_t from_stop_vertex_id = stops_to_vertex_ids_.at((*begin)->name).stop_go_id;
				double edge_weight = 0.0;
				auto curr_stop_it = begin;
				for (std::advance(curr_stop_it, 1); curr_stop_it != end; curr_stop_it++) {
					size_t to_stop_wait_vertex_id = stops_to_vertex_ids_.at((*curr_stop_it)->name).stop_wait_id;
					edge_weight += CalculateTime(transport_catalogue_.GetStopsDistance(*prev(curr_stop_it), *curr_stop_it) * 1.0, bus_velocity_);
					graph_.AddEdge({ from_stop_vertex_id, to_stop_wait_vertex_id, edge_weight, EdgeType::BUS, bus_name, std::distance(begin, curr_stop_it) });
				}

			}
		}

		void MakeGraph();
	};
}

