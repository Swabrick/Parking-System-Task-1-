#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include "parking.h"

class HttpServer {
public:
    HttpServer(ParkingSystem& parking, int port);
    bool start();

private:
    ParkingSystem& parking;
    int port;

    std::string handleRequest(const std::string& request);
    std::string route(const std::string& method, const std::string& path, const std::string& body);
    std::string serveFile(const std::string& path);
    std::string contentType(const std::string& path) const;
};

#endif
