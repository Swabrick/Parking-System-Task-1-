# Smart Parking Management System (C)

A console-based Smart Parking Management System written in C.

## Assignment Overview

The system manages a five-floor parking building with:

- 5 floors (Ground Floor is represented as Floor 1)
- 2 wings per floor: Wing A and Wing B
- 20 parking slots per wing
- 200 total parking slots

## Features

- View available parking spaces before entry
- Display availability by floor and wing
- Automatically assign the first available parking slot
- Record vehicle registration number and entry time
- Prevent duplicate entry of a vehicle already inside
- Search for currently parked vehicles
- Record vehicle exit time
- Calculate parking duration
- Calculate parking charges
- Release slots automatically on exit
- Increment and decrement available slots
- Display occupied slots
- Track total completed parking sessions and revenue

## Parking Fee Policy

| Parking Duration | Charge |
|---|---:|
| Less than 1 hour | FREE |
| Exactly 1 hour | KSh 50 |
| More than 1 hour | KSh 50 per started hour |

Examples:

- 59 minutes = FREE
- 1 hour = KSh 50
- 1 hour 1 minute = KSh 100
- 2 hours = KSh 100
- 2 hours 1 minute = KSh 150

## Project Structure

```text
smart-parking-system-c/
├── include/
│   ├── config.h
│   ├── input.h
│   ├── models.h
│   ├── parking.h
│   └── utils.h
│
├── src/
│   ├── input.c
│   ├── main.c
│   ├── parking.c
│   └── utils.c
│
├── .gitignore
├── Makefile
└── README.md
```

## Data Structure

The building is represented using a three-dimensional array:

```c
ParkingSlot slots[5][2][20];
```

Meaning:

```text
slots[FLOOR][WING][SLOT]
```

Examples:

- `slots[0][0][0]` = Floor 1, Wing A, Slot 1
- `slots[4][1][19]` = Floor 5, Wing B, Slot 20

## Slot Assignment Algorithm

The system assigns the first available slot in this order:

1. Floor 1 to Floor 5
2. Wing A to Wing B
3. Slot 1 to Slot 20

Therefore, the first vehicle is assigned `1A01`.

## Compile and Run

### Using GCC

```bash
gcc -Wall -Wextra -Wpedantic -std=c11 -Iinclude \
src/main.c src/parking.c src/input.c src/utils.c \
-o smart_parking
```

Run:

```bash
./smart_parking
```

### Using Make

```bash
make
make run
```

## Important Design Decisions

### Why a 3D Array?

The parking building has a natural hierarchy:

Floor -> Wing -> Slot

Therefore:

```c
slots[FLOORS][WINGS][SLOTS_PER_WING]
```

is easier to understand than treating all 200 spaces as one flat array.

### Why Structures?

Each parking slot needs related information:

- Occupancy status
- Vehicle registration number
- Entry time

A `struct` keeps these values together.

### Why Separate Files?

The project is divided into logical modules:

- `main.c` handles the program menu and flow.
- `parking.c` contains parking operations.
- `input.c` handles safe user input.
- `utils.c` contains reusable formatting functions.
- Header files expose functions and shared data structures.

This makes the program easier to read, test, explain, and maintain.

## Current Scope

This version stores information in memory while the program is running.

A future version could add:

- Database storage
- File persistence
- User authentication
- Payments
- Reserved parking
- Sensor integration
- A graphical or web interface
