#include "transport_catalogue.h"
#include "geo.h"

#include <unordered_set>
#include <cassert>

using namespace transport_catalogue;


void TransportCatalogue::AddStop(const std::string& stop_name, geo::Coordinates coordinates) {
    Stop stop{ stop_name, coordinates };
    stops_.emplace_back(stop);
    stop_name_to_data_.insert({ stops_.back().name, &stops_.back() });
    // добавляем остановку в индекс
    stop_name_to_buses_.insert({ &stops_.back(), {} });
}

void TransportCatalogue::SetStopDistances(const std::string_view from_stop_name, const std::string_view to_stop_name, int distance) {
    std::pair<const Stop*, const Stop*> stop_pair(stop_name_to_data_[from_stop_name], stop_name_to_data_[to_stop_name]);
    stop_pairs_to_distance_.insert({ stop_pair, distance });

}

int TransportCatalogue::GetStopsDistance(const Stop* from_stop, const Stop* to_stop) const {
    return stop_pairs_to_distance_.count({ from_stop, to_stop }) != 0 ?
        stop_pairs_to_distance_.at({ from_stop, to_stop })
        : stop_pairs_to_distance_.count({ to_stop, from_stop }) != 0
        ? stop_pairs_to_distance_.at({ to_stop, from_stop })
        : 0;
}

void TransportCatalogue::AddBus(const std::string& bus_name, const std::vector<std::string_view>& stops, bool is_roundtrip) {
    Bus current_bus;
    current_bus.name = bus_name;
    current_bus.is_roundtrip = is_roundtrip;
    for (const auto& stop_name : stops) {
        current_bus.route.emplace_back(FindStop(stop_name));
    }
    buses_.emplace_back(current_bus);
    bus_name_to_data_.insert({ buses_.back().name, &buses_.back() });
    for (const auto stop : buses_.back().route) {
        stop_name_to_buses_[stop].emplace(&buses_.back());
    }
}

Bus* TransportCatalogue::FindBus(const std::string_view bus_name) const {
    if (bus_name_to_data_.count(bus_name) == 0) {
        return nullptr;
    }
    return bus_name_to_data_.at(bus_name);
}

Stop* TransportCatalogue::FindStop(const std::string_view stop_name) const {
    if (stop_name_to_data_.count(stop_name) == 0) {
        return nullptr;
    }
    return stop_name_to_data_.at(stop_name);
}

BusInfo TransportCatalogue::GetBusInfo(const std::string_view bus_name) const {
    Bus* bus_info = FindBus(bus_name);
    assert(bus_info != nullptr);
    double geo_distance = 0.0;
    int route_length = 0;
    std::unordered_set<Stop*> unique_stops;
    for (size_t i = 0; i < bus_info->route.size() - 1; ++i) {
        geo_distance += geo::ComputeDistance({ bus_info->route[i]->coordinates.lat, bus_info->route[i]->coordinates.lng }, { bus_info->route[i + 1]->coordinates.lat, bus_info->route[i + 1]->coordinates.lng });
        route_length += GetStopsDistance(bus_info->route[i], bus_info->route[i + 1]);
        unique_stops.emplace(bus_info->route[i]);
    }
    double curvature = route_length / geo_distance;
    return { bus_info->route.size(), unique_stops.size(), route_length, curvature };
}

std::set<const Bus*, BusSetCmp> TransportCatalogue::GetStopInfo(const std::string_view stop_name) const {
    Stop* stop_ptr = FindStop(stop_name);
    assert(stop_ptr != nullptr);
    return stop_name_to_buses_.at(stop_ptr);
}

std::set<const Bus*, BusSetCmp> TransportCatalogue::GetBuses() const {
    std::set<const Bus*, BusSetCmp> result;
    for (const Bus& bus : buses_) {
        result.emplace(&bus);
    }
    return result;
}
