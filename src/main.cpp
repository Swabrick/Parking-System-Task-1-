#include <iostream>
#include "config.h"
#include "database.h"
#include "parking.h"
#include "http_server.h"

int main() {
    Database database(DATABASE_FILE);
    ParkingSystem parking(database);

    if (!parking.initialize()) {
        std::cerr << "Could not start the parking system.\n";
        std::cerr << parking.lastError() << "\n";
        return 1;
    }

    HttpServer server(parking, SERVER_PORT);
    if (!server.start()) {
        std::cerr << "Could not start the local web server.\n";
        return 1;
    }

    return 0;
}
