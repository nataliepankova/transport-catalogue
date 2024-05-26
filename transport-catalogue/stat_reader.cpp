#include "stat_reader.h"

#include <iomanip>
#include <algorithm>

using namespace std;
using namespace transport_catalogue;

void transport_stat::ParseAndPrint(const TransportCatalogue& tansport_catalogue, std::string_view request,
    std::ostream& output) {

    size_t first_nonspace_symb = request.find_first_not_of(' ');
    size_t last_command_symb = request.find_first_of(' ', first_nonspace_symb);
    std::string_view command = request.substr(first_nonspace_symb, last_command_symb);
    std::string_view descr = request.substr(request.find_first_not_of(' ', last_command_symb), request.find_last_not_of(' '));
    if (command  == "Bus"sv) {
        Bus* bus_info = tansport_catalogue.FindBus(descr);
        if (bus_info == nullptr) {
            output << command << " "s << descr << ": not found\n"s;
            return;
        }
        auto bus_stat = tansport_catalogue.GetBusInfo(descr);
        output << command << " "s << descr << ": "s << bus_stat.all_stops_count << " stops on route, "s << bus_stat.unique_stops_count << " unique stops, "s << setprecision(6) << bus_stat.route_length << " route length\n"s;
        return;
    }
    else if (command == "Stop"sv) {
        Stop* stop_ptr = tansport_catalogue.FindStop(descr);
        if (stop_ptr == nullptr) {
            output << command << " "s << descr << ": not found\n"s;
            return;
        }
        std::set<Bus*, TransportCatalogue::bus_set_cmp> buses_for_stop = tansport_catalogue.GetStopInfo(descr);
        if (buses_for_stop.empty()) {
            output << command << " "s << descr << ": no buses\n"s;
            return;
        }
        output << command << " "s << descr << ": buses "s;
        for (const auto bus_ptr : buses_for_stop) {
            output << bus_ptr->name << " "s;
        }
        output << endl;
    }

}
