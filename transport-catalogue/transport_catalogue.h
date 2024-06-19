#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <set>
#include <vector>

#include "geo.h"

namespace transport_catalogue {

	struct BusInfo {
		size_t all_stops_count;
		size_t unique_stops_count;
		int route_length;
		double curvature;
	};

	struct DistanceToStop{
		std::string stop_name;
		int distance;
	};

	struct Stop {
		std::string name;
		Coordinates coordinates;
	};

	struct Bus {
		std::string name;
		std::vector<Stop*> route;
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
		void AddStop(const std::string& stop_name, Coordinates coordinates);
		void SetStopsDistance(const std::string_view from_stop_name, const std::string_view to_stop_name, int distance);
		int GetStopsDistance(const Stop* from_stop, const Stop* to_stop) const;
		void AddBus(const std::string& bus_name, const std::vector<std::string_view>& stops);
		Stop* FindStop(const std::string_view stop_name) const;
		Bus* FindBus(const std::string_view bus_name) const;
		BusInfo GetBusInfo(const std::string_view bus_name) const;
		struct BusSetCmp {
			bool operator() (Bus* rhs, Bus* lhs) const {
				return rhs->name < lhs->name;
			}
		};
		std::set<Bus*, BusSetCmp> GetStopInfo(const std::string_view stop_name) const;

	private:
		std::deque<Stop> stops_;
		std::unordered_map<std::string_view, Stop*> stop_name_to_data_;
		std::unordered_map<Stop*, std::set<Bus*, BusSetCmp>> stop_name_to_buses_;
		std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopPairHasher> stop_pairs_to_distance_;
		std::deque<Bus> buses_;
		std::unordered_map<std::string_view, Bus*> bus_name_to_data_;
	};
}
