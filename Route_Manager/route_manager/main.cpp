#include <iostream>
#include "json_reader.h"
#include "transport_catalogue.h"
#include "transport_router.h"

using namespace std;

int main() {
    try {
        json::Document doc = json::Load(cin);
        TransportCatalogue catalogue;  
        json_reader::JsonReader reader;

        reader.LoadData(catalogue, doc);
        transport::TransportRouter router(catalogue, reader.GetRoutingSettings());
        json::Document result = reader.ProcessRequests(catalogue, doc, router);
        json::Print(result, cout);
    } catch (const std::exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return 1;
    }
}