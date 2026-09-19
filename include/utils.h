#ifndef UTILS_H
#define UTILS_H

#include <ctime>
#include <string>

std::string formatTime(std::time_t value);
std::string jsonEscape(const std::string& value);
std::string urlDecode(const std::string& value);
std::string normalizeVehicleNumber(std::string value);

#endif
