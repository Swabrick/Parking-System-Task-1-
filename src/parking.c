#include <stdio.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "input.h"
#include "parking.h"
#include "utils.h"

/* Counts available slots inside one wing. */
static int countAvailableInWing(
    const ParkingSystem *system,
    int floor,
    int wing
)
{
    int slot;
    int count = 0;

    for (slot = 0; slot < SLOTS_PER_WING; slot++) {
        if (system->slots[floor][wing][slot].occupied == 0) {
            count++;
        }
    }

    return count;
}

/* Stores a completed parking visit in memory. */
static void saveTransaction(
    ParkingSystem *system,
    const char *vehicleNumber,
    int floor,
    int wing,
    int slot,
    time_t entryTime,
    time_t exitTime,
    long durationSeconds,
    int chargedHours,
    double amount
)
{
    ParkingTransaction *transaction;

    /*
     * This assignment keeps up to MAX_TRANSACTIONS records.
     * The parking system itself continues working even when
     * the in-memory history becomes full.
     */
    if (system->transactionCount >= MAX_TRANSACTIONS) {
        return;
    }

    transaction =
        &system->transactions[system->transactionCount];

    strcpy(transaction->vehicleNumber, vehicleNumber);
    transaction->floor = floor;
    transaction->wing = wing;
    transaction->slot = slot;
    transaction->entryTime = entryTime;
    transaction->exitTime = exitTime;
    transaction->durationSeconds = durationSeconds;
    transaction->chargedHours = chargedHours;
    transaction->amount = amount;

    system->transactionCount++;
}

void initializeParkingSystem(ParkingSystem *system)
{
    int floor;
    int wing;
    int slot;

    for (floor = 0; floor < FLOORS; floor++) {
        for (wing = 0; wing < WINGS; wing++) {
            for (slot = 0; slot < SLOTS_PER_WING; slot++) {
                system->slots[floor][wing][slot].occupied = 0;
                system->slots[floor][wing][slot].vehicleNumber[0] = '\0';
                system->slots[floor][wing][slot].entryTime = 0;
            }
        }
    }

    system->availableSlots = TOTAL_SLOTS;
    system->totalVehiclesServed = 0;
    system->totalRevenue = 0.0;
    system->transactionCount = 0;
}

int findVehicleLocation(
    const ParkingSystem *system,
    const char *vehicleNumber,
    int *floor,
    int *wing,
    int *slot
)
{
    int f, w, s;

    for (f = 0; f < FLOORS; f++) {
        for (w = 0; w < WINGS; w++) {
            for (s = 0; s < SLOTS_PER_WING; s++) {
                if (system->slots[f][w][s].occupied == 1 &&
                    strcmp(
                        system->slots[f][w][s].vehicleNumber,
                        vehicleNumber
                    ) == 0) {
                    *floor = f;
                    *wing = w;
                    *slot = s;
                    return 1;
                }
            }
        }
    }

    return 0;
}

int findAvailableSlot(
    const ParkingSystem *system,
    int *floor,
    int *wing,
    int *slot
)
{
    int f, w, s;

    for (f = 0; f < FLOORS; f++) {
        for (w = 0; w < WINGS; w++) {
            for (s = 0; s < SLOTS_PER_WING; s++) {
                if (system->slots[f][w][s].occupied == 0) {
                    *floor = f;
                    *wing = w;
                    *slot = s;
                    return 1;
                }
            }
        }
    }

    return 0;
}

void displayParkingAvailability(const ParkingSystem *system)
{
    int floor;
    int wing;
    int floorAvailable;

    printf("\n========================================\n");
    printf("         PARKING AVAILABILITY\n");
    printf("========================================\n");
    printf("Total Slots:     %d\n", TOTAL_SLOTS);
    printf("Available Slots: %d\n", system->availableSlots);
    printf(
        "Occupied Slots:  %d\n",
        TOTAL_SLOTS - system->availableSlots
    );

    printf("\nAvailability by floor and wing:\n");

    for (floor = 0; floor < FLOORS; floor++) {
        floorAvailable = 0;

        printf("\nFloor %d\n", floor + 1);

        for (wing = 0; wing < WINGS; wing++) {
            int wingAvailable =
                countAvailableInWing(system, floor, wing);

            floorAvailable += wingAvailable;

            printf(
                "  Wing %c: %2d/%d available\n",
                wing == 0 ? 'A' : 'B',
                wingAvailable,
                SLOTS_PER_WING
            );
        }

        printf(
            "  Floor Total: %2d/40 available\n",
            floorAvailable
        );
    }

    printf("========================================\n");
}

void registerVehicleEntry(ParkingSystem *system)
{
    char vehicleNumber[MAX_VEHICLE_NUMBER];
    int floor, wing, slot;

    printf("\n========================================\n");
    printf("             VEHICLE ENTRY\n");
    printf("========================================\n");

    if (system->availableSlots == 0) {
        printf("Parking is FULL. No slots are available.\n");
        return;
    }

    printf(
        "Available parking spaces before entry: %d\n",
        system->availableSlots
    );

    printf("Enter vehicle registration number: ");
    readLine(vehicleNumber, sizeof(vehicleNumber));
    normalizeVehicleNumber(vehicleNumber);

    if (strlen(vehicleNumber) == 0) {
        printf("Vehicle registration number cannot be empty.\n");
        return;
    }

    if (findVehicleLocation(
            system,
            vehicleNumber,
            &floor,
            &wing,
            &slot
        )) {
        printf(
            "Entry denied: vehicle %s is already parked at ",
            vehicleNumber
        );
        printSlotCode(floor, wing, slot);
        printf(".\n");
        return;
    }

    if (!findAvailableSlot(
            system,
            &floor,
            &wing,
            &slot
        )) {
        printf("No available slot could be assigned.\n");
        return;
    }

    system->slots[floor][wing][slot].occupied = 1;
    strcpy(
        system->slots[floor][wing][slot].vehicleNumber,
        vehicleNumber
    );
    system->slots[floor][wing][slot].entryTime = time(NULL);

    system->availableSlots--;

    printf("\nParking successfully assigned.\n");
    printf("Vehicle:      %s\n", vehicleNumber);
    printf("Parking Slot: ");
    printSlotCode(floor, wing, slot);
    printf("\nEntry Time:   ");
    printFormattedTime(
        system->slots[floor][wing][slot].entryTime
    );
    printf(
        "\nAvailable Slots Remaining: %d\n",
        system->availableSlots
    );
}

void registerVehicleExit(ParkingSystem *system)
{
    char vehicleNumber[MAX_VEHICLE_NUMBER];
    int floor, wing, slot;
    time_t entryTime;
    time_t exitTime;
    long durationSeconds;
    int chargedHours = 0;
    double amount = 0.0;

    printf("\n========================================\n");
    printf("              VEHICLE EXIT\n");
    printf("========================================\n");

    printf("Enter vehicle registration number: ");
    readLine(vehicleNumber, sizeof(vehicleNumber));
    normalizeVehicleNumber(vehicleNumber);

    if (!findVehicleLocation(
            system,
            vehicleNumber,
            &floor,
            &wing,
            &slot
        )) {
        printf(
            "Vehicle %s was not found in the parking building.\n",
            vehicleNumber
        );
        return;
    }

    entryTime = system->slots[floor][wing][slot].entryTime;
    exitTime = time(NULL);

    durationSeconds = (long)difftime(exitTime, entryTime);

    /*
     * Charging policy:
     * Less than one hour -> Free
     * One hour or more -> KSh 50 per started hour.
     *
     * Examples:
     * 60 minutes       -> KSh 50
     * 61 minutes       -> KSh 100
     * 120 minutes      -> KSh 100
     * 121 minutes      -> KSh 150
     */
    if (durationSeconds >= FREE_PARKING_SECONDS) {
        chargedHours =
            (int)(
                (durationSeconds + FREE_PARKING_SECONDS - 1)
                / FREE_PARKING_SECONDS
            );

        amount = chargedHours * HOURLY_RATE;
    }

    printf("\n========================================\n");
    printf("            PARKING RECEIPT\n");
    printf("========================================\n");
    printf("Vehicle:      %s\n", vehicleNumber);

    printf("Parking Slot: ");
    printSlotCode(floor, wing, slot);
    printf("\n");

    printf("Entry Time:   ");
    printFormattedTime(entryTime);
    printf("\n");

    printf("Exit Time:    ");
    printFormattedTime(exitTime);
    printf("\n");

    printf("Duration:     ");
    printDuration(durationSeconds);
    printf("\n");

    if (amount == 0.0) {
        printf("Amount Due:   FREE\n");
    } else {
        printf("Charged Time: %d hour(s)\n", chargedHours);
        printf("Amount Due:   KSh %.2f\n", amount);
    }

    printf("========================================\n");

    saveTransaction(
        system,
        vehicleNumber,
        floor,
        wing,
        slot,
        entryTime,
        exitTime,
        durationSeconds,
        chargedHours,
        amount
    );

    /*
     * Release the slot only after all receipt information
     * has been captured and the transaction has been saved.
     */
    system->slots[floor][wing][slot].occupied = 0;
    system->slots[floor][wing][slot].vehicleNumber[0] = '\0';
    system->slots[floor][wing][slot].entryTime = 0;

    system->availableSlots++;
    system->totalVehiclesServed++;
    system->totalRevenue += amount;

    printf("Parking slot released successfully.\n");
    printf(
        "Available parking spaces: %d\n",
        system->availableSlots
    );
}

void searchVehicle(const ParkingSystem *system)
{
    char vehicleNumber[MAX_VEHICLE_NUMBER];
    int floor, wing, slot;

    printf("\nEnter vehicle registration number to search: ");
    readLine(vehicleNumber, sizeof(vehicleNumber));
    normalizeVehicleNumber(vehicleNumber);

    if (findVehicleLocation(
            system,
            vehicleNumber,
            &floor,
            &wing,
            &slot
        )) {
        printf("\nVehicle Found\n");
        printf("Vehicle: %s\n", vehicleNumber);
        printf("Location: ");
        printSlotCode(floor, wing, slot);
        printf("\nEntry Time: ");
        printFormattedTime(
            system->slots[floor][wing][slot].entryTime
        );
        printf("\n");
    } else {
        printf(
            "Vehicle %s is not currently parked here.\n",
            vehicleNumber
        );
    }
}

void displayOccupiedSlots(const ParkingSystem *system)
{
    int floor, wing, slot;
    int occupiedCount = 0;

    printf("\n========================================\n");
    printf("           OCCUPIED PARKING SLOTS\n");
    printf("========================================\n");

    for (floor = 0; floor < FLOORS; floor++) {
        for (wing = 0; wing < WINGS; wing++) {
            int sectionHasVehicle = 0;

            for (slot = 0; slot < SLOTS_PER_WING; slot++) {
                if (system->slots[floor][wing][slot].occupied) {
                    if (!sectionHasVehicle) {
                        printf(
                            "\nFloor %d - Wing %c\n",
                            floor + 1,
                            wing == 0 ? 'A' : 'B'
                        );
                        sectionHasVehicle = 1;
                    }

                    printf("  ");
                    printSlotCode(floor, wing, slot);
                    printf(
                        " -> %s\n",
                        system->slots[floor][wing][slot].vehicleNumber
                    );

                    occupiedCount++;
                }
            }
        }
    }

    if (occupiedCount == 0) {
        printf("No vehicles are currently parked.\n");
    }

    printf("\nTotal Occupied Slots: %d\n", occupiedCount);
    printf("========================================\n");
}

void displayStatistics(const ParkingSystem *system)
{
    int floor;
    int mostOccupiedFloor = 0;
    int highestOccupiedCount = -1;

    printf("\n========================================\n");
    printf("           PARKING STATISTICS\n");
    printf("========================================\n");

    printf(
        "Vehicles Successfully Served: %d\n",
        system->totalVehiclesServed
    );
    printf(
        "Completed Transactions Stored: %d\n",
        system->transactionCount
    );
    printf(
        "Total Revenue: KSh %.2f\n",
        system->totalRevenue
    );

    printf("\nCurrent occupancy by floor:\n");

    for (floor = 0; floor < FLOORS; floor++) {
        int wingA =
            SLOTS_PER_WING - countAvailableInWing(system, floor, 0);
        int wingB =
            SLOTS_PER_WING - countAvailableInWing(system, floor, 1);
        int total = wingA + wingB;

        printf(
            "Floor %d: %d/40 occupied\n",
            floor + 1,
            total
        );

        if (total > highestOccupiedCount) {
            highestOccupiedCount = total;
            mostOccupiedFloor = floor;
        }
    }

    printf(
        "\nCurrently Most Occupied Floor: Floor %d (%d vehicle(s))\n",
        mostOccupiedFloor + 1,
        highestOccupiedCount
    );

    printf("========================================\n");
}
