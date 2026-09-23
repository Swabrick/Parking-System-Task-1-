# Data Structures Used in the Smart Parking Management System

## 1. Overview

The system uses C++ structures, vectors, strings, enumerations and time values to keep the parking information organised.

The main structures are:

- `ParkingSlot`
- `ParkingTransaction`
- `PricingSettings`
- `BarrierState`
- `std::vector`
- `std::string`
- Enumerations
- `std::time_t`

## 2. ParkingSlot

`ParkingSlot` represents one physical parking space.

```cpp
struct ParkingSlot {
    int floor;
    int wing;
    int number;
    SlotStatus status;
    std::string vehicleNumber;
    std::time_t entryTime;
};
```

The fields store the floor, wing, slot number, current state, vehicle registration and entry time.

There are 200 `ParkingSlot` objects in the system.

## 3. Vector of parking slots

The parking spaces are stored in:

```cpp
std::vector<ParkingSlot> slots;
```

The vector contains 200 elements arranged logically in this order:

```text
Floor 1
    Wing A
        Slot 1 to 20
    Wing B
        Slot 1 to 20
Floor 2
    Wing A
        Slot 1 to 20
    Wing B
        Slot 1 to 20
...
Floor 5
    Wing B
        Slot 1 to 20
```

The first available element is selected when a vehicle enters. This keeps the implementation simple without needing a separate multi-dimensional array.

## 4. ParkingTransaction

`ParkingTransaction` stores the details of a recorded parking visit. A charged exit can remain pending until payment is confirmed.

```cpp
struct ParkingTransaction {
    long long id;
    std::string vehicleNumber;
    int floor;
    int wing;
    int slot;
    std::time_t entryTime;
    std::time_t exitTime;
    long long durationMinutes;
    int amount;
    SessionStatus status;
    PaymentStatus paymentStatus;
    PaymentMethod paymentMethod;
};
```

Transactions are stored in:

```cpp
std::vector<ParkingTransaction> transactions;
```

## 5. PricingSettings

`PricingSettings` stores the current parking limits and fees.

```cpp
struct PricingSettings {
    int freeLimitMinutes;
    int firstLimitMinutes;
    int secondLimitMinutes;
    int thirdLimitMinutes;
    int firstFee;
    int secondFee;
    int thirdFee;
    int maximumFee;
};
```

The settings are saved in the local database so rates can be changed from the Management page without changing source code.

## 6. BarrierState

`BarrierState` stores the current software state of the two barriers.

```cpp
struct BarrierState {
    bool entranceOpen;
    bool exitOpen;
    bool automaticMode;
};
```

Automatic operation is the default. Manual open and close functions are provided as an override when the automatic path is unavailable.

## 7. PaymentStatus and PaymentMethod

Payment status is represented with: 

```cpp
enum class PaymentStatus { Pending, Paid };
```

The payment method is represented with:

```cpp
enum class PaymentMethod { NotSelected, Cash, Card, Mpesa };
```

A charged exit starts as `Pending`. After the operator confirms collection, it becomes `Paid`.

## 8. Strings

Vehicle registration numbers are stored using `std::string`. The registration is normalised before searching or saving so differences in spaces and letter case do not create duplicate active vehicles.

## 9. Time values

The system uses `std::time_t` for entry and exit times. The difference between the two values is used to calculate parking duration.

## 10. How the structures work together

```text
ParkingSlot
    |
    | current vehicle and entry time
    v
Active vehicle
    |
    | exit creates
    v
ParkingTransaction
    |
    +-- duration
    +-- amount
    +-- payment status
    +-- payment method

PricingSettings
    |
    +-- controls fee calculation

BarrierState
    |
    +-- controls automatic/manual barrier state
```

The vectors hold the working data while the local database stores the same information permanently on the computer.
