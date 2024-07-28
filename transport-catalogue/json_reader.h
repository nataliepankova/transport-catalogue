#pragma once
#include <string_view>
#include <vector>
#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

using namespace json;

namespace json_reader {
	class JsonReader {
	public:
		JsonReader(std::istream& input)
			: json_doc_(Load(input)) {}

		void ApplyCommands(transport_catalogue::TransportCatalogue& catalogue, renderer::MapRenderer& renderer) const;

	private:
		Document json_doc_;
		void ApplyBaseRequests(const Array& request_array, transport_catalogue::TransportCatalogue& catalogue) const;
		void ApplyStatRequests(const Array& request_array, transport_catalogue::TransportCatalogue& catalogue, renderer::MapRenderer& renderer) const;
		void ApplyRenderSettings(const Dict& render_settings, renderer::MapRenderer& renderer) const;
		Node PrepareBusStat(const Dict& request, transport_catalogue::TransportCatalogue& catalogue) const;
		Node PrepareStopStat(const Dict& request, transport_catalogue::TransportCatalogue& catalogue) const; 
		Node PrepareMap(const Dict& request, std::set<const Bus*, BusSetCmp>& buses, renderer::MapRenderer& renderer) const;
	};
	namespace detail {
		std::vector<DistanceToStop> ParseDistanceToStop(const Node& stop_info);
		std::vector<std::string_view> ParseRoute(const Node& route, bool is_roundtrip);

	}
}