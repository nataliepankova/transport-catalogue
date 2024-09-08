#include <iostream>
#include <fstream>
#include <string>

#include "json_reader.h"

using namespace std;
using namespace json_reader;
using namespace renderer;
using namespace transport_router;
using namespace transport_catalogue;

int main() {

    JsonReader json_reader{ cin };
    TransportCatalogue catalogue;
    json_reader.ApplyBaseRequests(catalogue);
    MapRenderer renderer;
    json_reader.ApplyRenderSettings(renderer);
    optional<TranspRouteParams> params = json_reader.GetRoutingSettings();
    TransportRouter router{ catalogue, params };

    json_reader.ApplyStatRequests(catalogue, renderer, router);
}
