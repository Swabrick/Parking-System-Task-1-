#ifndef PARKING_H
#define PARKING_H

#include "models.h"

void initializeParkingSystem(ParkingSystem *system);

void displayParkingAvailability(const ParkingSystem *system);
void displayOccupiedSlots(const ParkingSystem *system);
void searchVehicle(const ParkingSystem *system);
void displayStatistics(const ParkingSystem *system);

void registerVehicleEntry(ParkingSystem *system);
void registerVehicleExit(ParkingSystem *system);

/*
 * Finds an occupied slot containing the vehicle number.
 * Returns 1 when found and 0 when not found.
 */
int findVehicleLocation(
    const ParkingSystem *system,
    const char *vehicleNumber,
    int *floor,
    int *wing,
    int *slot
);

/*
 * Finds the first available slot.
 * Assignment priority:
 * Floor 1 -> Floor 5
 * Wing A -> Wing B
 * Slot 1 -> Slot 20
 */
int findAvailableSlot(
    const ParkingSystem *system,
    int *floor,
    int *wing,
    int *slot
);

#endif
