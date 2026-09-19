#ifndef MODELS_H
#define MODELS_H

#include <ctime>
#include <string>
#include "config.h"

enum class SlotStatus { Available = 0, Occupied = 1 };

enum class SessionStatus { Active = 0, Completed = 1 };

struct ParkingSlot {
    int floor = 0;
    int wing = 0;
    int number = 0;
    SlotStatus status = SlotStatus::Available;
    std::string vehicleNumber;
    std::time_t entryTime = 0;
};

struct ParkingTransaction {
    long long id = 0;
    std::string vehicleNumber;
    int floor = 0;
    int wing = 0;
    int slot = 0;
    std::time_t entryTime = 0;
    std::time_t exitTime = 0;
    long long durationMinutes = 0;
    int amount = 0;
    SessionStatus status = SessionStatus::Completed;
};

#endif
