#ifndef PARKING_H
#define PARKING_H

#include <ctime>
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
    bool confirmPayment(long long transactionId, PaymentMethod paymentMethod,
                        ParkingTransaction& transaction, std::string& message);
    bool findVehicle(const std::string& vehicleNumber, ParkingSlot& slot) const;

    bool addBlacklist(const std::string& vehicleNumber, std::string& message);
    bool removeBlacklist(const std::string& vehicleNumber, std::string& message);
    bool isBlacklisted(const std::string& vehicleNumber) const;

    bool updatePricing(const PricingSettings& settings, std::string& message);
    bool updateTransaction(long long id, int amount, PaymentStatus paymentStatus,
                           PaymentMethod paymentMethod, std::string& message);

    const std::vector<ParkingSlot>& getSlots() const;
    const std::vector<ParkingTransaction>& getTransactions() const;
    const std::vector<std::string>& getBlacklist() const;
    const PricingSettings& getPricing() const;
    const BarrierState& getBarriers() const;

    // These functions are used by management for manual barrier override.
    void openEntranceBarrier();
    void closeEntranceBarrier();
    void openExitBarrier();
    void closeExitBarrier();
    void setAutomaticBarrierMode(bool enabled);

    int availableSlots() const;
    int occupiedSlots() const;
    int calculateFee(long long durationMinutes) const;
    std::string slotCode(const ParkingSlot& slot) const;
    std::string lastError() const;

private:
    static constexpr int BARRIER_OPEN_SECONDS = 5;

    Database& database;
    std::vector<ParkingSlot> slots;
    std::vector<ParkingTransaction> transactions;
    std::vector<std::string> blacklist;
    PricingSettings pricing;
    mutable BarrierState barriers;
    mutable std::time_t entranceOpenedAt = 0;
    mutable std::time_t exitOpenedAt = 0;

    ParkingSlot* findVehicleSlot(const std::string& vehicleNumber);
    ParkingSlot* findAvailableSlot();
    ParkingTransaction* findTransaction(long long id);
    void buildSlots();
    void trimAndUpper(std::string& value) const;
    void updateAutomaticBarriers() const;
    void automaticOpenEntranceBarrier();
    void automaticOpenExitBarrier();
    bool saveChanges();
};

#endif
