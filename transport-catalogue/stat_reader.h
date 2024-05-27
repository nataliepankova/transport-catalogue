#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace transport_stat {

    namespace detail {
        void PrintBusInfo(const transport_catalogue::TransportCatalogue& tansport_catalogue, std::string_view bus_descr, std::string_view command, std::ostream& output);
        void PrintStopInfo(const transport_catalogue::TransportCatalogue& tansport_catalogue, std::string_view stop_descr, std::string_view command, std::ostream& output);
    }

    void ParseAndPrint(const transport_catalogue::TransportCatalogue& tansport_catalogue, std::string_view request,
        std::ostream& output);
}
