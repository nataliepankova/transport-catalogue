#include <iostream>
#include <fstream>
#include <string>

#include "json_reader.h"

using namespace std;
using namespace json_reader;
using namespace renderer;
using namespace transport_catalogue;

int main() {
    TransportCatalogue catalogue;

    //ifstream read_file;
    //read_file.open("input.txt");
    JsonReader json_reader{ cin };
    MapRenderer renderer;
    json_reader.ApplyCommands(catalogue, renderer);
}