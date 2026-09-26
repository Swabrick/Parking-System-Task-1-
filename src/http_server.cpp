#include "http_server.h"
#include "config.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <ctime>
#include <map>

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
        const char* text = status == 200 ? "OK" : (status == 404 ? "Not Found" : "Bad Request");
        std::ostringstream out;
        out << "HTTP/1.1 " << status << " " << text << "\r\n"
            << "Content-Type: application/json; charset=utf-8\r\n"
            << "Cache-Control: no-store\r\n"
            << "Connection: close\r\n\r\n" << body;
        return out.str();
    }

    std::string formValue(const std::string& body, const std::string& key) {
        std::string token = key + "=";
        auto start = body.find(token);
        if (start == std::string::npos) return {};
        start += token.size();
        auto end = body.find('&', start);
        return urlDecode(body.substr(start, end == std::string::npos ? std::string::npos : end - start));
    }

    int formInt(const std::string& body, const std::string& key, int fallback = 0) {
        try { return std::stoi(formValue(body, key)); }
        catch (...) { return fallback; }
    }

    long long formLong(const std::string& body, const std::string& key, long long fallback = 0) {
        try { return std::stoll(formValue(body, key)); }
        catch (...) { return fallback; }
    }

    PaymentMethod paymentMethodFromText(const std::string& value) {
        if (value == "cash") return PaymentMethod::Cash;
        if (value == "card") return PaymentMethod::Card;
        if (value == "mpesa") return PaymentMethod::Mpesa;
        return PaymentMethod::NotSelected;
    }

    std::string paymentMethodName(PaymentMethod method) {
        switch (method) {
            case PaymentMethod::Cash: return "Cash";
            case PaymentMethod::Card: return "Card";
            case PaymentMethod::Mpesa: return "M-Pesa";
            default: return "Not selected";
        }
    }

    std::string slotCode(const ParkingTransaction& tx) {
        std::ostringstream out;
        out << tx.floor << (tx.wing == 0 ? "A" : "B") << (tx.slot < 10 ? "0" : "") << tx.slot;
        return out.str();
    }
}

HttpServer::HttpServer(ParkingSystem& parking, int port) : parking(parking), port(port) {}

std::string HttpServer::contentType(const std::string& path) const {
    if (path.size() >= 5 && path.compare(path.size() - 5, 5, ".html") == 0) return "text/html; charset=utf-8";
    if (path.size() >= 4 && path.compare(path.size() - 4, 4, ".css") == 0) return "text/css; charset=utf-8";
    if (path.size() >= 3 && path.compare(path.size() - 3, 3, ".js") == 0) return "application/javascript; charset=utf-8";
    return "text/plain; charset=utf-8";
}

std::string HttpServer::serveFile(const std::string& path) {
    std::string safe = path == "/" ? "web/index.html" : "web" + path;
    if (safe.find("..") != std::string::npos) return "";

    std::ifstream file(safe, std::ios::binary);
    if (!file) return "";

    std::ostringstream content;
    content << file.rdbuf();
    return "HTTP/1.1 200 OK\r\nContent-Type: " + contentType(safe) +
           "\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n" + content.str();
}

std::string HttpServer::route(const std::string& method, const std::string& path, const std::string& body) {
    if (method == "GET" && path == "/api/status") {
        const auto& p = parking.getPricing();
        const auto& b = parking.getBarriers();
        std::ostringstream out;
        out << "{\"total\":" << TOTAL_SLOTS
            << ",\"available\":" << parking.availableSlots()
            << ",\"occupied\":" << parking.occupiedSlots()
            << ",\"transactions\":" << parking.getTransactions().size()
            << ",\"blacklisted\":" << parking.getBlacklist().size()
            << ",\"entranceOpen\":" << (b.entranceOpen ? "true" : "false")
            << ",\"exitOpen\":" << (b.exitOpen ? "true" : "false")
            << ",\"automaticBarriers\":" << (b.automaticMode ? "true" : "false")
            << ",\"rateFree\":" << p.freeLimitMinutes
            << ",\"rate1Limit\":" << p.firstLimitMinutes
            << ",\"rate2Limit\":" << p.secondLimitMinutes
            << ",\"rate3Limit\":" << p.thirdLimitMinutes
            << ",\"fee1\":" << p.firstFee
            << ",\"fee2\":" << p.secondFee
            << ",\"fee3\":" << p.thirdFee
            << ",\"feeMax\":" << p.maximumFee << "}";
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
                << "\",\"vehicle\":\"" << jsonEscape(slot.vehicleNumber)
                << "\",\"entry\":\"" << jsonEscape(formatTime(slot.entryTime)) << "\"}";
        }
        out << "]";
        return jsonResponse(out.str());
    }

    if (method == "GET" && path == "/api/active") {
        std::ostringstream out;
        out << "[";
        bool first = true;
        for (const auto& slot : parking.getSlots()) {
            if (slot.status != SlotStatus::Occupied) continue;
            if (!first) out << ',';
            first = false;

            long long minutes = static_cast<long long>((std::time(nullptr) - slot.entryTime + 59) / 60);
            bool paymentPending = false;
            for (const auto& tx : parking.getTransactions()) {
                if (tx.vehicleNumber == slot.vehicleNumber && tx.paymentStatus == PaymentStatus::Pending) {
                    paymentPending = true;
                    break;
                }
            }

            out << "{\"vehicle\":\"" << jsonEscape(slot.vehicleNumber)
                << "\",\"slot\":\"" << parking.slotCode(slot)
                << "\",\"floor\":" << slot.floor
                << ",\"wing\":\"" << (slot.wing == 0 ? "A" : "B")
                << "\",\"entry\":\"" << jsonEscape(formatTime(slot.entryTime))
                << "\",\"minutes\":" << minutes
                << ",\"estimatedFee\":" << parking.calculateFee(minutes)
                << ",\"paymentPending\":" << (paymentPending ? "true" : "false") << "}";
        }
        out << "]";
        return jsonResponse(out.str());
    }

    if (method == "GET" && path == "/api/transactions") {
        std::ostringstream out;
        out << "[";
        bool first = true;
        for (auto it = parking.getTransactions().rbegin();
             it != parking.getTransactions().rend() &&
             std::distance(parking.getTransactions().rbegin(), it) < 100; ++it) {
            if (!first) out << ',';
            first = false;
            out << "{\"id\":" << it->id
                << ",\"vehicle\":\"" << jsonEscape(it->vehicleNumber)
                << "\",\"slot\":\"" << slotCode(*it)
                << "\",\"duration\":" << it->durationMinutes
                << ",\"amount\":" << it->amount
                << ",\"paymentStatus\":\"" << (it->paymentStatus == PaymentStatus::Paid ? "Paid" : "Pending")
                << "\",\"paymentMethod\":\"" << paymentMethodName(it->paymentMethod)
                << "\",\"entry\":\"" << jsonEscape(formatTime(it->entryTime))
                << "\",\"exit\":\"" << jsonEscape(formatTime(it->exitTime)) << "\"}";
        }
        out << "]";
        return jsonResponse(out.str());
    }

    if (method == "GET" && path == "/api/analytics") {
        long long revenue = 0;
        long long duration = 0;
        int paid = 0;
        int free = 0;
        int pending = 0;
        int peakHour[24]{};
        int floorUse[FLOORS + 1]{};

        for (const auto& tx : parking.getTransactions()) {
            duration += tx.durationMinutes;
            if (tx.amount == 0) {
                ++free;
            } else if (tx.paymentStatus == PaymentStatus::Paid) {
                ++paid;
                revenue += tx.amount;
            } else {
                ++pending;
            }

            std::tm* local = std::localtime(&tx.entryTime);
            if (local) ++peakHour[local->tm_hour];
            if (tx.floor >= 1 && tx.floor <= FLOORS) ++floorUse[tx.floor];
        }

        int peak = 0;
        for (int i = 1; i < 24; ++i) {
            if (peakHour[i] > peakHour[peak]) peak = i;
        }

        double average = parking.getTransactions().empty()
            ? 0.0
            : static_cast<double>(duration) / parking.getTransactions().size();

        std::ostringstream out;
        out << "{\"revenue\":" << revenue
            << ",\"completed\":" << parking.getTransactions().size()
            << ",\"paid\":" << paid
            << ",\"free\":" << free
            << ",\"pending\":" << pending
            << ",\"averageMinutes\":" << average
            << ",\"peakHour\":" << peak
            << ",\"floorUse\":[";
        for (int floor = 1; floor <= FLOORS; ++floor) {
            if (floor > 1) out << ',';
            out << floorUse[floor];
        }
        out << "]}";
        return jsonResponse(out.str());
    }

    if (method == "GET" && path == "/api/blacklist") {
        std::ostringstream out;
        out << "[";
        for (std::size_t i = 0; i < parking.getBlacklist().size(); ++i) {
            if (i) out << ',';
            out << "\"" << jsonEscape(parking.getBlacklist()[i]) << "\"";
        }
        out << "]";
        return jsonResponse(out.str());
    }

    auto pricingJson = [&]() {
        const auto& p = parking.getPricing();
        std::ostringstream out;
        out << "{\"freeLimit\":" << p.freeLimitMinutes
            << ",\"firstLimit\":" << p.firstLimitMinutes
            << ",\"secondLimit\":" << p.secondLimitMinutes
            << ",\"thirdLimit\":" << p.thirdLimitMinutes
            << ",\"firstFee\":" << p.firstFee
            << ",\"secondFee\":" << p.secondFee
            << ",\"thirdFee\":" << p.thirdFee
            << ",\"maximumFee\":" << p.maximumFee << "}";
        return out.str();
    };

    if (method == "GET" && path == "/api/settings") return jsonResponse(pricingJson());

    if (method == "GET" && path == "/api/barriers") {
        const auto& b = parking.getBarriers();
        std::ostringstream out;
        out << "{\"entrance\":\"" << (b.entranceOpen ? "open" : "closed")
            << "\",\"exit\":\"" << (b.exitOpen ? "open" : "closed")
            << "\",\"automatic\":" << (b.automaticMode ? "true" : "false") << "}";
        return jsonResponse(out.str());
    }

    if (method == "POST" && path == "/api/entry") {
        std::string message;
        const std::string vehicle = formValue(body, "vehicle");
        if (!parking.enterVehicle(vehicle, message)) {
            return jsonResponse("{\"success\":false,\"message\":\"" + jsonEscape(message) + "\"}", 400);
        }

        ParkingSlot slot;
        parking.findVehicle(vehicle, slot);
        const auto& barriers = parking.getBarriers();
        std::ostringstream out;
        out << "{\"success\":true,\"message\":\"" << jsonEscape(message)
            << "\",\"slot\":\"" << parking.slotCode(slot)
            << "\",\"barrier\":\"" << (barriers.entranceOpen ? "open" : "manual") << "\"}";
        return jsonResponse(out.str());
    }

    if (method == "POST" && path == "/api/exit") {
        ParkingTransaction tx;
        std::string message;
        const std::string vehicle = formValue(body, "vehicle");
        if (!parking.exitVehicle(vehicle, tx, message)) {
            return jsonResponse("{\"success\":false,\"message\":\"" + jsonEscape(message) + "\"}", 400);
        }

        std::ostringstream out;
        const auto& barriers = parking.getBarriers();
        out << "{\"success\":true,\"message\":\"" << jsonEscape(message)
            << "\",\"transactionId\":" << tx.id
            << ",\"vehicle\":\"" << jsonEscape(tx.vehicleNumber)
            << "\",\"slot\":\"" << slotCode(tx)
            << "\",\"duration\":" << tx.durationMinutes
            << ",\"amount\":" << tx.amount
            << ",\"paymentStatus\":\"" << (tx.amount == 0 ? "Paid" : "Pending")
            << "\",\"paymentOptions\":[\"Cash\",\"Card\",\"M-Pesa\"]"
            << ",\"barrier\":\"" << (barriers.exitOpen ? "open" : "waiting")
            << "\",\"entry\":\"" << jsonEscape(formatTime(tx.entryTime))
            << "\",\"exit\":\"" << jsonEscape(formatTime(tx.exitTime)) << "\"}";
        return jsonResponse(out.str());
    }

    if (method == "POST" && path == "/api/payment/confirm") {
        const long long id = formLong(body, "id");
        const PaymentMethod paymentMethod = paymentMethodFromText(formValue(body, "paymentMethod"));
        ParkingTransaction tx;
        std::string message;

        if (!parking.confirmPayment(id, paymentMethod, tx, message)) {
            return jsonResponse("{\"success\":false,\"message\":\"" + jsonEscape(message) + "\"}", 400);
        }

        const auto& barriers = parking.getBarriers();
        std::ostringstream out;
        out << "{\"success\":true,\"message\":\"" << jsonEscape(message)
            << "\",\"transactionId\":" << tx.id
            << ",\"vehicle\":\"" << jsonEscape(tx.vehicleNumber)
            << "\",\"slot\":\"" << slotCode(tx)
            << "\",\"duration\":" << tx.durationMinutes
            << ",\"amount\":" << tx.amount
            << ",\"paymentStatus\":\"Paid\""
            << ",\"paymentMethod\":\"" << paymentMethodName(tx.paymentMethod)
            << "\",\"barrier\":\"" << (barriers.exitOpen ? "open" : "manual")
            << "\",\"entry\":\"" << jsonEscape(formatTime(tx.entryTime))
            << "\",\"exit\":\"" << jsonEscape(formatTime(tx.exitTime)) << "\"}";
        return jsonResponse(out.str());
    }

    if (method == "POST" && path == "/api/blacklist/add") {
        std::string message;
        bool ok = parking.addBlacklist(formValue(body, "vehicle"), message);
        return jsonResponse("{\"success\":" + std::string(ok ? "true" : "false") +
                            ",\"message\":\"" + jsonEscape(message) + "\"}", ok ? 200 : 400);
    }

    if (method == "POST" && path == "/api/blacklist/remove") {
        std::string message;
        bool ok = parking.removeBlacklist(formValue(body, "vehicle"), message);
        return jsonResponse("{\"success\":" + std::string(ok ? "true" : "false") +
                            ",\"message\":\"" + jsonEscape(message) + "\"}", ok ? 200 : 400);
    }

    if (method == "POST" && path == "/api/settings/pricing") {
        PricingSettings p = parking.getPricing();
        p.freeLimitMinutes = formInt(body, "freeLimit", p.freeLimitMinutes);
        p.firstLimitMinutes = formInt(body, "firstLimit", p.firstLimitMinutes);
        p.secondLimitMinutes = formInt(body, "secondLimit", p.secondLimitMinutes);
        p.thirdLimitMinutes = formInt(body, "thirdLimit", p.thirdLimitMinutes);
        p.firstFee = formInt(body, "firstFee", p.firstFee);
        p.secondFee = formInt(body, "secondFee", p.secondFee);
        p.thirdFee = formInt(body, "thirdFee", p.thirdFee);
        p.maximumFee = formInt(body, "maximumFee", p.maximumFee);
        std::string message;
        bool ok = parking.updatePricing(p, message);
        return jsonResponse("{\"success\":" + std::string(ok ? "true" : "false") +
                            ",\"message\":\"" + jsonEscape(message) + "\"}", ok ? 200 : 400);
    }

    if (method == "POST" && path == "/api/transactions/update") {
        const long long id = formLong(body, "id");
        const int amount = formInt(body, "amount");
        const PaymentStatus status = formValue(body, "paymentStatus") == "paid"
            ? PaymentStatus::Paid : PaymentStatus::Pending;
        const PaymentMethod paymentMethod = paymentMethodFromText(formValue(body, "paymentMethod"));
        std::string message;
        bool ok = parking.updateTransaction(id, amount, status, paymentMethod, message);
        return jsonResponse("{\"success\":" + std::string(ok ? "true" : "false") +
                            ",\"message\":\"" + jsonEscape(message) + "\"}", ok ? 200 : 400);
    }

    if (method == "POST" && path == "/api/barrier") {
        const std::string barrier = formValue(body, "barrier");
        const std::string action = formValue(body, "action");

        if (barrier == "entrance") {
            if (action == "open") parking.openEntranceBarrier();
            else if (action == "close") parking.closeEntranceBarrier();
            else return jsonResponse("{\"success\":false,\"message\":\"Unknown entrance barrier action.\"}", 400);
        } else if (barrier == "exit") {
            if (action == "open") parking.openExitBarrier();
            else if (action == "close") parking.closeExitBarrier();
            else return jsonResponse("{\"success\":false,\"message\":\"Unknown exit barrier action.\"}", 400);
        } else if (barrier == "mode") {
            if (action != "automatic" && action != "manual") {
                return jsonResponse("{\"success\":false,\"message\":\"Unknown barrier mode.\"}", 400);
            }
            parking.setAutomaticBarrierMode(action == "automatic");
        } else {
            return jsonResponse("{\"success\":false,\"message\":\"Unknown barrier control.\"}", 400);
        }

        return jsonResponse("{\"success\":true,\"message\":\"Barrier control updated.\"}");
    }

    // Kept as a simple endpoint for the interface to explain that direct
    // electronic payment processing is not connected to this local system.
    if (method == "POST" && path == "/api/payment-prompt") {
        return jsonResponse("{\"success\":true,\"message\":\"Electronic payment integration is coming soon. Cash can be collected and confirmed manually by the operator.\"}");
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

    auto query = path.find('?');
    if (query != std::string::npos) path = path.substr(0, query);

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

    std::cout << "Saint Hotel Parking System running at http://localhost:" << port << "\n";

    while (true) {
        sockaddr_in client{};
#ifdef _WIN32
        int length = sizeof(client);
#else
        socklen_t length = sizeof(client);
#endif
        SocketType clientSocket = accept(server, reinterpret_cast<sockaddr*>(&client), &length);
        if (clientSocket == INVALID_SOCKET_VALUE) continue;

        char buffer[16384]{};
#ifdef _WIN32
        int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
#else
        int received = static_cast<int>(recv(clientSocket, buffer, sizeof(buffer) - 1, 0));
#endif
        if (received > 0) {
            std::string response = handleRequest(std::string(buffer, received));
            send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
        }
        closeSocket(clientSocket);
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return true;
}
