#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <set>
#include <vector>

namespace transport_catalogue {

	struct BusInfo {
		size_t all_stops_count;
		size_t unique_stops_count;
		double route_length;
	};

	struct Stop {
		std::string name;
		double latitude;
		double longitude;
	};

	struct Bus {
		std::string name;
		std::vector<Stop*> route;
	};

	class TransportCatalogue {

	public:
		void AddStop(const std::string& stop_name, const double lat, const double lng);
		void AddBus(const std::string& bus_name, const std::vector<std::string_view>& stops);
		Stop* FindStop(const std::string_view stop_name) const;
		Bus* FindBus(const std::string_view bus_name) const;
		BusInfo GetBusInfo(const std::string_view bus_name) const;
		struct bus_set_cmp {
			bool operator() (Bus* rhs, Bus* lhs) const {
				return rhs->name < lhs->name;
			}
		};
		std::set<Bus*, bus_set_cmp> GetStopInfo(const std::string_view stop_name) const;

	private:
		std::deque<Stop> stops_;
		std::unordered_map<std::string_view, Stop*> stop_name_to_data_;
		std::unordered_map<Stop*, std::set<Bus*, bus_set_cmp>> stop_name_to_buses_;
		std::deque<Bus> buses_;
		std::unordered_map<std::string_view, Bus*> bus_name_to_data_;
	};
}
