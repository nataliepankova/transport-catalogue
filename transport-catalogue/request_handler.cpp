/*#include "request_handler.h"


RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& db, const renderer::MapRenderer& renderer) 
	:db_(db), renderer_(renderer) {}

// Возвращает информацию о маршруте (запрос Bus)
std::optional<transport_catalogue::BusInfo> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
	if (db_.FindBus(bus_name) == nullptr) {
		return std::nullopt;
	}
	return db_.GetBusInfo(bus_name);
}

// Возвращает маршруты, проходящие через остановку
const std::set<Bus*, BusSetCmp>* RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
	if (db_.FindStop(stop_name) == nullptr) {
		return nullptr;
	}
	return &db_.GetStopInfo(stop_name);
}

void RequestHandler::RenderMap() const {
	renderer_.RenderMap(db_.GetBuses());
}
*/