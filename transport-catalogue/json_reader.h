#pragma once
#include <string_view>
#include <vector>
#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

using namespace json;

namespace json_reader {
	class JsonReader {
	public:
		JsonReader(std::istream& input)
			: json_doc_(Load(input)) {}

		transport_router::TranspRouteParams GetRoutingSettings() const;
		void ApplyBaseRequests(transport_catalogue::TransportCatalogue& catalogue) const;
		void ApplyRenderSettings(renderer::MapRenderer& renderer) const;
		void ApplyStatRequests(transport_catalogue::TransportCatalogue& catalogue, renderer::MapRenderer& renderer, transport_router::TransportRouter& router) const;

	private:
		Document json_doc_;
		void AddStopsToCatalogue(const Array& request_array, transport_catalogue::TransportCatalogue& catalogue) const;
		void SetStopDistancesInCatalogue(const Array& request_array, transport_catalogue::TransportCatalogue& catalogue) const;
		void AddBusesToCatalogue(const Array& request_array, transport_catalogue::TransportCatalogue& catalogue) const;
		svg::Color CreateColorFromArray(const Array& shades, renderer::MapRenderer& renderer) const;
		Node PrepareBusStat(const Dict& request, transport_catalogue::TransportCatalogue& catalogue) const;
		Node PrepareStopStat(const Dict& request, transport_catalogue::TransportCatalogue& catalogue) const;
		Node PrepareMap(const Dict& request, std::set<const Bus*, BusSetCmp>& buses, renderer::MapRenderer& renderer) const;
		Node PrepareRouteStat(const Dict& request, transport_router::TransportRouter& router) const;
	};
	namespace detail {
		std::vector<DistanceToStop> ParseDistanceToStop(const Node& stop_info);
		std::vector<std::string_view> ParseRoute(const Node& route, bool is_roundtrip);

	}
}
