# Data Structures Used in the Smart Parking Management System

## 1. Overview

The Smart Parking Management System uses several data structures to organize parking spaces vehicles and parking records. The structures are implemented in C++ and are designed to match the physical layout of the parking building.

The main data structures used are:

- Structures (`struct`)
- Arrays
- Vectors
- Strings
- Enumerations (`enum`)
- Date and time objects

These structures work together to manage the parking spaces and store information about vehicles and parking sessions.

## 2. Parking Slot Structure

Each parking space is represented by a `ParkingSlot` structure.

```cpp
struct ParkingSlot {
    int floor;
    char wing;
    int slotNumber;
    bool occupied;
    string vehicleNumber;
};
```

### Implementation

The structure stores the basic information needed for one parking space:

- `floor` stores the floor number.
- `wing` stores either Wing A or Wing B.
- `slotNumber` identifies the individual space.
- `occupied` shows whether the space is available.
- `vehicleNumber` stores the registration number of the vehicle using the space.

This makes it possible to treat each parking space as one complete record.

## 3. Three-Dimensional Parking Array

The parking building contains:

- 5 floors
- 2 wings per floor
- 20 spaces per wing

A three-dimensional array can represent this layout:

```cpp
ParkingSlot parkingSlots[5][2][20];
```

The dimensions represent:

```text
parkingSlots[floor][wing][slot]
```

For example:

```cpp
parkingSlots[0][0][0]
```

represents Floor 1 Wing A Slot 1.

The indexes start from zero in C++ so:

```text
0 = Floor 1
1 = Floor 2
2 = Floor 3
3 = Floor 4
4 = Floor 5

0 = Wing A
1 = Wing B

0 = Slot 1
1 = Slot 2
...
19 = Slot 20
```

This gives a total of:

```text
5 × 2 × 20 = 200 parking spaces
```

The three-dimensional array is useful because its structure directly represents the physical structure of the parking building.

## 4. Vehicle Information

Vehicle registration numbers are stored using the C++ `string` data type.

Example:

```cpp
string vehicleNumber = "KDA 123A";
```

Strings are suitable because registration numbers contain letters and numbers.

The system uses the vehicle number to:

- Identify a vehicle.
- Search for a parked vehicle.
- Assign a vehicle to a parking space.
- Process vehicle exit.
- Connect a parking session with a vehicle.

## 5. Parking Session Structure

A parking session represents one visit to the parking facility.

A session can contain information such as:

```cpp
struct ParkingSession {
    string vehicleNumber;
    int floor;
    char wing;
    int slotNumber;
    time_t entryTime;
    time_t exitTime;
    long durationSeconds;
    double amount;
};
```

### Implementation

When a vehicle enters the system records its entry time.

When the vehicle leaves the system records the exit time and calculates the parking duration.

The session then contains the information needed to calculate and record the parking charge.

## 6. Vectors

The system can use C++ `vector` to store collections of records that can grow as the system operates.

Example:

```cpp
vector<ParkingSession> sessions;
```

Unlike a fixed-size array a vector can increase its size when new parking sessions are added.

This is useful for parking history because the number of completed sessions can increase over time.

For example:

```text
Vehicle 1 → Parking Session
Vehicle 2 → Parking Session
Vehicle 3 → Parking Session
        ↓
      vector
```

Each completed session can be added to the vector.

## 7. Enumeration for Parking Status

An enumeration can be used to represent the state of a parking space.

Example:

```cpp
enum class ParkingStatus {
    AVAILABLE,
    OCCUPIED
};
```

Instead of storing unclear numeric values such as `0` and `1` the program can use meaningful names.

For example:

```cpp
slot.status = ParkingStatus::AVAILABLE;
```

or:

```cpp
slot.status = ParkingStatus::OCCUPIED;
```

This makes the code easier to understand and reduces the chance of using the wrong status value.

## 8. Date and Time Data

The system needs to record when vehicles enter and leave.

C++ time functionality can be used for this:

```cpp
time_t entryTime;
time_t exitTime;
```

When a vehicle enters:

```cpp
entryTime = time(nullptr);
```

When the vehicle exits:

```cpp
exitTime = time(nullptr);
```

The difference between the two times gives the parking duration.

The duration is then used to determine the parking fee.

## 9. Database Records

The local database stores persistent information about the parking system.

The main records are organized around:

```text
Vehicle
ParkingSlot
ParkingSession
```

A vehicle can have multiple parking sessions over time.

A parking slot can also be used by different vehicles at different times.

The parking session connects the vehicle with the parking slot and records the entry and exit information.

## 10. Relationship Between the Data Structures

The structures work together as follows:

```text
ParkingSlot
     |
     | assigned to
     v
  Vehicle
     |
     | creates
     v
ParkingSession
     |
     | stores
     v
Entry Time
Exit Time
Duration
Amount
```

The three-dimensional parking array manages the physical parking layout while structures represent individual records. Vectors manage collections of records and the local database provides permanent storage.

## 11. Why These Data Structures Were Used

The data structures were selected because they fit the requirements of the system.

### Struct

Used to group related information into one object such as a parking space or parking session.

### Array

Used to represent the fixed physical parking layout of 5 floors 2 wings and 20 spaces per wing.

### Vector

Used for collections where the number of records can increase during operation.

### String

Used for vehicle registration numbers and other text values.

### Enum

Used to represent parking status using clear names.

### Time data

Used to calculate how long a vehicle remained in the parking facility.

Together these structures provide a simple way to organize and process the information required by the Smart Parking Management System.
