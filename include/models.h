#ifndef MODELS_H
#define MODELS_H

#include <ctime>
#include <string>
#include "config.h"

// Small enums keep status values clear throughout the backend.
enum class SlotStatus { Available = 0, Occupied = 1 };
enum class SessionStatus { Active = 0, Completed = 1 };
enum class PaymentStatus { Pending = 0, Paid = 1 };

enum class PaymentMethod {
    NotSelected = 0,
    Cash = 1,
    Card = 2,
    Mpesa = 3
};

struct ParkingSlot {
    int floor = 0;
    int wing = 0;
    int number = 0;
    SlotStatus status = SlotStatus::Available;
    std::string vehicleNumber;
    std::time_t entryTime = 0;
};

// A charged exit remains pending until payment is confirmed.
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
    PaymentStatus paymentStatus = PaymentStatus::Paid;
    PaymentMethod paymentMethod = PaymentMethod::NotSelected;
};

struct PricingSettings {
    int freeLimitMinutes = 30;
    int firstLimitMinutes = 120;
    int secondLimitMinutes = 240;
    int thirdLimitMinutes = 360;
    int firstFee = 50;
    int secondFee = 100;
    int thirdFee = 300;
    int maximumFee = 500;
};

// BarrierState is the software representation of the two gates.
struct BarrierState {
    bool entranceOpen = false;
    bool exitOpen = false;
    bool automaticMode = true;
};

#endif
