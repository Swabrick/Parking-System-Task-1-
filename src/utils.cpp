#include "utils.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

std::string formatTime(std::time_t value) {
    if (value == 0) return "-";
    std::tm tmValue{};
#ifdef _WIN32
    localtime_s(&tmValue, &value);
#else
    localtime_r(&value, &tmValue);
#endif
    std::ostringstream out;
    out << std::put_time(&tmValue, "%d %b %Y, %H:%M:%S");
    return out.str();
}

std::string jsonEscape(const std::string& value) {
    std::string out;
    for (char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
        }
    }
    return out;
}

std::string urlDecode(const std::string& value) {
    std::string out;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            int number = 0;
            std::istringstream hex(value.substr(i + 1, 2));
            hex >> std::hex >> number;
            out += static_cast<char>(number);
            i += 2;
        } else if (value[i] == '+') {
            out += ' ';
        } else {
            out += value[i];
        }
    }
    return out;
}

std::string normalizeVehicleNumber(std::string value) {
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c);
    }), value.end());
    for (char& c : value) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
}
