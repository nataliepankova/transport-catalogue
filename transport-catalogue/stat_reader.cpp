#include "stat_reader.h"

#include <iomanip>
#include <algorithm>

using namespace std;
using namespace transport_catalogue;

void transport_stat::detail::PrintBusInfo(const TransportCatalogue& tansport_catalogue, std::string_view bus_descr, std::string_view command,
    std::ostream& output) {
    Bus* bus_info = tansport_catalogue.FindBus(bus_descr);
    if (bus_info == nullptr) {
        output << command << " "s << bus_descr << ": not found\n"s;
        return;
    }
    auto bus_stat = tansport_catalogue.GetBusInfo(bus_descr);
    output << command << " "s << bus_descr << ": "s <<
        bus_stat.all_stops_count << " stops on route, "s <<
        bus_stat.unique_stops_count << " unique stops, "s <<
        setprecision(6) << bus_stat.route_length << " route length\n"s;
}

void transport_stat::detail::PrintStopInfo(const TransportCatalogue& tansport_catalogue, std::string_view stop_descr, std::string_view command, std::ostream& output) {
    Stop* stop_ptr = tansport_catalogue.FindStop(stop_descr);
    if (stop_ptr == nullptr) {
        output << command << " "s << stop_descr << ": not found\n"s;
        return;
    }
    std::set<Bus*, TransportCatalogue::BusSetCmp> buses_for_stop = tansport_catalogue.GetStopInfo(stop_descr);
    if (buses_for_stop.empty()) {
        output << command << " "s << stop_descr << ": no buses\n"s;
        return;
    }
    output << command << " "s << stop_descr << ": buses "s;
    for (const auto bus_ptr : buses_for_stop) {
        output << bus_ptr->name << " "s;
    }
    output << endl;
}

void transport_stat::ParseAndPrint(const TransportCatalogue& tansport_catalogue, std::string_view request,
    std::ostream& output) {

    size_t first_nonspace_symb = request.find_first_not_of(' ');
    size_t last_command_symb = request.find_first_of(' ', first_nonspace_symb);
    std::string_view command = request.substr(first_nonspace_symb, last_command_symb);
    std::string_view descr = request.substr(request.find_first_not_of(' ', last_command_symb), request.find_last_not_of(' '));
    if (command  == "Bus"sv) {
        transport_stat::detail::PrintBusInfo(tansport_catalogue, descr, command, output);
    } else if (command == "Stop"sv) {
        transport_stat::detail::PrintStopInfo(tansport_catalogue, descr, command, output);
    }

}
