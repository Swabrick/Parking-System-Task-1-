#include <stdio.h>
#include "input.h"
#include "parking.h"

static void displayMenu(void)
{
    printf("\n========================================\n");
    printf("      SMART PARKING MANAGEMENT SYSTEM\n");
    printf("========================================\n");
    printf("1. View Parking Availability\n");
    printf("2. Register Vehicle Entry\n");
    printf("3. Register Vehicle Exit\n");
    printf("4. Search for Vehicle\n");
    printf("5. Display Occupied Slots\n");
    printf("6. Display System Statistics\n");
    printf("7. Exit Program\n");
    printf("========================================\n");
}

int main(void)
{
    ParkingSystem system;
    int choice;

    initializeParkingSystem(&system);

    printf("\nWelcome to the Smart Parking Management System.\n");

    do {
        displayMenu();

        choice = readIntInRange(
            "Choose an option (1-7): ",
            1,
            7
        );

        switch (choice) {
            case 1:
                displayParkingAvailability(&system);
                break;

            case 2:
                registerVehicleEntry(&system);
                break;

            case 3:
                registerVehicleExit(&system);
                break;

            case 4:
                searchVehicle(&system);
                break;

            case 5:
                displayOccupiedSlots(&system);
                break;

            case 6:
                displayStatistics(&system);
                break;

            case 7:
                printf(
                    "\nThank you for using the Smart Parking System.\n"
                );
                break;
        }

    } while (choice != 7);

    return 0;
}
