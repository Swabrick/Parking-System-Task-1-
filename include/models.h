#ifndef MODELS_H
#define MODELS_H

#include <time.h>
#include "config.h"

/* Represents one physical parking slot. */
typedef struct {
    int occupied;                               /* 0 = available, 1 = occupied */
    char vehicleNumber[MAX_VEHICLE_NUMBER];
    time_t entryTime;
} ParkingSlot;

/* Represents a completed parking visit. */
typedef struct {
    char vehicleNumber[MAX_VEHICLE_NUMBER];
    int floor;
    int wing;
    int slot;
    time_t entryTime;
    time_t exitTime;
    long durationSeconds;
    int chargedHours;
    double amount;
} ParkingTransaction;

/* Stores the entire parking system state. */
typedef struct {
    ParkingSlot slots[FLOORS][WINGS][SLOTS_PER_WING];

    int availableSlots;
    int totalVehiclesServed;
    double totalRevenue;

    ParkingTransaction transactions[MAX_TRANSACTIONS];
    int transactionCount;
} ParkingSystem;

#endif
