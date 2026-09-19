#ifndef PARKING_H
#define PARKING_H

#include <string>
#include <vector>
#include "models.h"
#include "database.h"

class ParkingSystem {
public:
    explicit ParkingSystem(Database& database);

    bool initialize();
    bool enterVehicle(const std::string& vehicleNumber, std::string& message);
    bool exitVehicle(const std::string& vehicleNumber, ParkingTransaction& transaction, std::string& message);
    bool findVehicle(const std::string& vehicleNumber, ParkingSlot& slot) const;

    const std::vector<ParkingSlot>& getSlots() const;
    const std::vector<ParkingTransaction>& getTransactions() const;
    int availableSlots() const;
    int occupiedSlots() const;
    int calculateFee(long long durationMinutes) const;
    std::string slotCode(const ParkingSlot& slot) const;
    std::string lastError() const;

private:
    Database& database;
    std::vector<ParkingSlot> slots;
    std::vector<ParkingTransaction> transactions;

    ParkingSlot* findVehicleSlot(const std::string& vehicleNumber);
    ParkingSlot* findAvailableSlot();
    void buildSlots();
    void trimAndUpper(std::string& value) const;
};

#endif
