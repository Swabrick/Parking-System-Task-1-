#ifndef CONFIG_H
#define CONFIG_H

#define FLOORS 5
#define WINGS 2
#define SLOTS_PER_WING 20
#define TOTAL_SLOTS (FLOORS * WINGS * SLOTS_PER_WING)
#define MAX_VEHICLE_NUMBER 20
#define MAX_TRANSACTIONS 5000
#define SERVER_PORT 8080
#define DATABASE_FILE "storage/parking.db"

#endif
