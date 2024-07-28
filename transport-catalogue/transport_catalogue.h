#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <set>
#include <vector>

#include "domain.h"

namespace transport_catalogue {

	struct BusInfo {
		size_t all_stops_count;
		size_t unique_stops_count;
		int route_length;
		double curvature;
	};

	class StopPairHasher {
	public:
		size_t operator() (std::pair<const Stop*, const Stop*> stops_pair) const {
			size_t hash_first = hasher_(stops_pair.first);
			size_t hash_second = hasher_(stops_pair.second);
			return (hash_first + hash_second) * 37;
		}
		bool operator ==(std::pair<const Stop*, const Stop*> other) {
			return *this == other;
		}

	private:
		std::hash<const Stop*> hasher_;
	};

	class TransportCatalogue {

	public:
		void AddStop(const std::string& stop_name, geo::Coordinates coordinates);
		void SetStopDistances(const std::string_view from_stop_name, const std::string_view to_stop_name, int distance);
		int GetStopsDistance(const Stop* from_stop, const Stop* to_stop) const;
		void AddBus(const std::string& bus_name, const std::vector<std::string_view>& stops, bool is_roundtrip);
		Stop* FindStop(const std::string_view stop_name) const;
		Bus* FindBus(const std::string_view bus_name) const;
		BusInfo GetBusInfo(const std::string_view bus_name) const;
		std::set<const Bus*, BusSetCmp> GetStopInfo(const std::string_view stop_name) const;

		std::set<const Bus*, BusSetCmp> GetBuses() const;

	private:
		std::deque<Stop> stops_;
		std::unordered_map<std::string_view, Stop*> stop_name_to_data_;
		std::unordered_map<Stop*, std::set<const Bus*, BusSetCmp>> stop_name_to_buses_;
		std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopPairHasher> stop_pairs_to_distance_;
		std::deque<Bus> buses_;
		std::unordered_map<std::string_view, Bus*> bus_name_to_data_;
	};
}