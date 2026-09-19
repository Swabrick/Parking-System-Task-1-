#include "http_server.h"
#include "config.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using SocketType = SOCKET;
const SocketType INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
using SocketType = int;
const SocketType INVALID_SOCKET_VALUE = -1;
#endif

namespace {
    void closeSocket(SocketType socket) {
#ifdef _WIN32
        closesocket(socket);
#else
        close(socket);
#endif
    }

    std::string jsonResponse(const std::string& body, int status = 200) {
        std::string text = status == 200 ? "OK" : (status == 404 ? "Not Found" : "Bad Request");
        std::ostringstream out;
        out << "HTTP/1.1 " << status << " " << text << "\r\n"
            << "Content-Type: application/json; charset=utf-8\r\n"
            << "Cache-Control: no-store\r\n"
            << "Connection: close\r\n\r\n" << body;
        return out.str();
    }
}

HttpServer::HttpServer(ParkingSystem& parking, int port) : parking(parking), port(port) {}

std::string HttpServer::contentType(const std::string& path) const {
    if (path.size() >= 5 && path.compare(path.size()-5, 5, ".html") == 0) return "text/html; charset=utf-8";
    if (path.size() >= 4 && path.compare(path.size()-4, 4, ".css") == 0) return "text/css; charset=utf-8";
    if (path.size() >= 3 && path.compare(path.size()-3, 3, ".js") == 0) return "application/javascript; charset=utf-8";
    return "text/plain; charset=utf-8";
}

std::string HttpServer::serveFile(const std::string& path) {
    std::string safe = path == "/" ? "web/index.html" : "web" + path;
    if (safe.find("..") != std::string::npos) return "";
    std::ifstream file(safe, std::ios::binary);
    if (!file) return "";
    std::ostringstream content;
    content << file.rdbuf();
    return "HTTP/1.1 200 OK\r\nContent-Type: " + contentType(safe) + "\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n" + content.str();
}

std::string HttpServer::route(const std::string& method, const std::string& path, const std::string& body) {
    if (method == "GET" && path == "/api/status") {
        std::ostringstream out;
        out << "{\"total\":" << TOTAL_SLOTS
            << ",\"available\":" << parking.availableSlots()
            << ",\"occupied\":" << parking.occupiedSlots()
            << ",\"transactions\":" << parking.getTransactions().size() << "}";
        return jsonResponse(out.str());
    }

    if (method == "GET" && path == "/api/slots") {
        std::ostringstream out;
        out << "[";
        bool first = true;
        for (const auto& slot : parking.getSlots()) {
            if (!first) out << ',';
            first = false;
            out << "{\"floor\":" << slot.floor
                << ",\"wing\":\"" << (slot.wing == 0 ? "A" : "B")
                << "\",\"number\":" << slot.number
                << ",\"code\":\"" << parking.slotCode(slot)
                << "\",\"status\":\"" << (slot.status == SlotStatus::Available ? "available" : "occupied")
                << "\",\"vehicle\":\"" << jsonEscape(slot.vehicleNumber) << "\"}";
        }
        out << "]";
        return jsonResponse(out.str());
    }

    if (method == "GET" && path == "/api/transactions") {
        std::ostringstream out;
        out << "[";
        bool first = true;
        for (auto it = parking.getTransactions().rbegin(); it != parking.getTransactions().rend() && std::distance(parking.getTransactions().rbegin(), it) < 50; ++it) {
            if (!first) out << ',';
            first = false;
            out << "{\"id\":" << it->id
                << ",\"vehicle\":\"" << jsonEscape(it->vehicleNumber)
                << "\",\"slot\":\"" << it->floor << (it->wing == 0 ? "A" : "B") << (it->slot < 10 ? "0" : "") << it->slot
                << "\",\"duration\":" << it->durationMinutes
                << ",\"amount\":" << it->amount
                << ",\"entry\":\"" << jsonEscape(formatTime(it->entryTime))
                << "\",\"exit\":\"" << jsonEscape(formatTime(it->exitTime)) << "\"}";
        }
        out << "]";
        return jsonResponse(out.str());
    }

    auto getFormValue = [&](const std::string& key) {
        std::string token = key + "=";
        auto start = body.find(token);
        if (start == std::string::npos) return std::string();
        start += token.size();
        auto end = body.find('&', start);
        return urlDecode(body.substr(start, end == std::string::npos ? std::string::npos : end - start));
    };

    if (method == "POST" && (path == "/api/entry" || path == "/api/exit")) {
        std::string vehicle = getFormValue("vehicle");
        if (path == "/api/entry") {
            std::string message;
            if (!parking.enterVehicle(vehicle, message)) return jsonResponse("{\"success\":false,\"message\":\"" + jsonEscape(message) + "\"}", 400);
            ParkingSlot slot;
            parking.findVehicle(vehicle, slot);
            return jsonResponse("{\"success\":true,\"message\":\"" + jsonEscape(message) + "\",\"slot\":\"" + parking.slotCode(slot) + "\"}");
        }
        ParkingTransaction tx;
        std::string message;
        if (!parking.exitVehicle(vehicle, tx, message)) return jsonResponse("{\"success\":false,\"message\":\"" + jsonEscape(message) + "\"}", 400);
        std::ostringstream out;
        out << "{\"success\":true,\"message\":\"" << jsonEscape(message)
            << "\",\"vehicle\":\"" << jsonEscape(tx.vehicleNumber)
            << "\",\"slot\":\"" << tx.floor << (tx.wing == 0 ? "A" : "B") << (tx.slot < 10 ? "0" : "") << tx.slot
            << "\",\"duration\":" << tx.durationMinutes << ",\"amount\":" << tx.amount << "}";
        return jsonResponse(out.str());
    }

    return jsonResponse("{\"message\":\"Not found\"}", 404);
}

std::string HttpServer::handleRequest(const std::string& request) {
    std::istringstream input(request);
    std::string method;
    std::string path;
    std::string version;
    input >> method >> path >> version;

    auto headerEnd = request.find("\r\n\r\n");
    std::string body = headerEnd == std::string::npos ? "" : request.substr(headerEnd + 4);

    if (path.rfind("/api/", 0) == 0) return route(method, path, body);
    if (method == "GET") {
        std::string file = serveFile(path);
        if (!file.empty()) return file;
    }
    return "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nNot Found";
}

bool HttpServer::start() {
#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return false;
#endif

    SocketType server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET_VALUE) return false;

    int option = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&option), sizeof(option));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(static_cast<unsigned short>(port));

    if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 || listen(server, 10) < 0) {
        closeSocket(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    std::cout << "Smart Parking System running at http://localhost:" << port << "\n";
    std::cout << "Press Ctrl+C to stop the server.\n";

    while (true) {
        SocketType client = accept(server, nullptr, nullptr);
        if (client == INVALID_SOCKET_VALUE) continue;

        std::string request;
        char buffer[8192];
        int received = 0;
        do {
#ifdef _WIN32
            received = recv(client, buffer, sizeof(buffer), 0);
#else
            received = static_cast<int>(recv(client, buffer, sizeof(buffer), 0));
#endif
            if (received > 0) request.append(buffer, received);
            if (request.find("\r\n\r\n") != std::string::npos) {
                auto headerEnd = request.find("\r\n\r\n");
                auto contentLengthPos = request.find("Content-Length:");
                if (contentLengthPos == std::string::npos) break;
                auto lineEnd = request.find("\r\n", contentLengthPos);
                std::size_t length = std::stoul(request.substr(contentLengthPos + 15, lineEnd - contentLengthPos - 15));
                if (request.size() >= headerEnd + 4 + length) break;
            }
        } while (received > 0);

        std::string response = handleRequest(request);
        send(client, response.c_str(), static_cast<int>(response.size()), 0);
        closeSocket(client);
    }
}
