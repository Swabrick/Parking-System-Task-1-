#include "parking.h"
#include "config.h"
#include "utils.h"
#include <algorithm>
#include <cctype>
#include <ctime>

// ParkingSystem contains the main rules for vehicle movement, payments,
// parking rates, blacklist checks and barrier control.
ParkingSystem::ParkingSystem(Database& database) : database(database) {}

void ParkingSystem::buildSlots() {
    slots.clear();
    slots.reserve(TOTAL_SLOTS);

    // Slots are stored in the same order used by the parking layout:
    // floor, wing, then slot number.
    for (int floor = 1; floor <= FLOORS; ++floor) {
        for (int wing = 0; wing < WINGS; ++wing) {
            for (int number = 1; number <= SLOTS_PER_WING; ++number) {
                ParkingSlot slot;
                slot.floor = floor;
                slot.wing = wing;
                slot.number = number;
                slots.push_back(slot);
            }
        }
    }
}

bool ParkingSystem::initialize() {
    if (!database.initialize()) return false;
    if (!database.load(slots, transactions, blacklist, pricing)) return false;

    // A new installation has no slot records yet, so build the 200 spaces.
    if (slots.size() != TOTAL_SLOTS) buildSlots();
    return saveChanges();
}

void ParkingSystem::trimAndUpper(std::string& value) const {
    value = normalizeVehicleNumber(value);
}

ParkingSlot* ParkingSystem::findVehicleSlot(const std::string& vehicleNumber) {
    for (auto& slot : slots) {
        if (slot.status == SlotStatus::Occupied && slot.vehicleNumber == vehicleNumber) return &slot;
    }
    return nullptr;
}

ParkingSlot* ParkingSystem::findAvailableSlot() {
    for (auto& slot : slots) {
        if (slot.status == SlotStatus::Available) return &slot;
    }
    return nullptr;
}

ParkingTransaction* ParkingSystem::findTransaction(long long id) {
    for (auto& transaction : transactions) {
        if (transaction.id == id) return &transaction;
    }
    return nullptr;
}

bool ParkingSystem::saveChanges() {
    return database.save(slots, transactions, blacklist, pricing);
}

bool ParkingSystem::enterVehicle(const std::string& rawVehicleNumber, std::string& message) {
    std::string vehicleNumber = rawVehicleNumber;
    trimAndUpper(vehicleNumber);

    if (vehicleNumber.empty() || vehicleNumber.size() >= MAX_VEHICLE_NUMBER) {
        message = "Enter a valid vehicle registration number.";
        return false;
    }
    if (isBlacklisted(vehicleNumber)) {
        message = "This vehicle is blacklisted and cannot enter.";
        return false;
    }
    if (findVehicleSlot(vehicleNumber)) {
        message = "This vehicle is already inside the parking facility.";
        return false;
    }

    ParkingSlot* slot = findAvailableSlot();
    if (!slot) {
        message = "The parking facility is currently full.";
        return false;
    }

    slot->status = SlotStatus::Occupied;
    slot->vehicleNumber = vehicleNumber;
    slot->entryTime = std::time(nullptr);

    if (!saveChanges()) {
        slot->status = SlotStatus::Available;
        slot->vehicleNumber.clear();
        slot->entryTime = 0;
        message = database.error();
        return false;
    }

    // Automatic mode is the normal path. Management can still use the manual
    // barrier controls when the automatic path is unavailable.
    if (barriers.automaticMode) {
        automaticOpenEntranceBarrier();
        message = "Vehicle assigned to slot " + slotCode(*slot) + ". Entrance barrier opened automatically.";
    } else {
        message = "Vehicle assigned to slot " + slotCode(*slot) + ". Open the entrance barrier manually.";
    }
    return true;
}

int ParkingSystem::calculateFee(long long durationMinutes) const {
    if (durationMinutes <= pricing.freeLimitMinutes) return 0;
    if (durationMinutes <= pricing.firstLimitMinutes) return pricing.firstFee;
    if (durationMinutes <= pricing.secondLimitMinutes) return pricing.secondFee;
    if (durationMinutes <= pricing.thirdLimitMinutes) return pricing.thirdFee;
    return pricing.maximumFee;
}

bool ParkingSystem::exitVehicle(const std::string& rawVehicleNumber,
                                ParkingTransaction& transaction,
                                std::string& message) {
    std::string vehicleNumber = rawVehicleNumber;
    trimAndUpper(vehicleNumber);
    ParkingSlot* slot = findVehicleSlot(vehicleNumber);

    if (!slot) {
        message = "Vehicle not found in the parking facility.";
        return false;
    }

    // Do not create a second payment request for a vehicle already waiting for
    // the operator to confirm payment.
    for (const auto& existing : transactions) {
        if (existing.vehicleNumber == vehicleNumber &&
            existing.paymentStatus == PaymentStatus::Pending) {
            transaction = existing;
            message = "Payment is already pending for this vehicle. Confirm the payment to complete the exit.";
            return true;
        }
    }

    const std::time_t exitTime = std::time(nullptr);
    const long long seconds = static_cast<long long>(std::difftime(exitTime, slot->entryTime));
    const long long minutes = (seconds + 59) / 60;

    transaction.id = transactions.empty() ? 1 : transactions.back().id + 1;
    transaction.vehicleNumber = vehicleNumber;
    transaction.floor = slot->floor;
    transaction.wing = slot->wing;
    transaction.slot = slot->number;
    transaction.entryTime = slot->entryTime;
    transaction.exitTime = exitTime;
    transaction.durationMinutes = minutes;
    transaction.amount = calculateFee(minutes);
    transaction.status = SessionStatus::Completed;
    transaction.paymentStatus = transaction.amount == 0 ? PaymentStatus::Paid : PaymentStatus::Pending;
    transaction.paymentMethod = PaymentMethod::NotSelected;

    transactions.push_back(transaction);

    // A free exit is complete immediately. A charged exit keeps the space
    // occupied until the operator confirms that payment was collected.
    if (transaction.amount == 0) {
        slot->status = SlotStatus::Available;
        slot->vehicleNumber.clear();
        slot->entryTime = 0;
    }

    if (!saveChanges()) {
        transactions.pop_back();
        message = database.error();
        return false;
    }

    if (transaction.amount == 0) {
        if (barriers.automaticMode) {
            automaticOpenExitBarrier();
            message = "Exit recorded. No payment was required, so the exit barrier opened automatically.";
        } else {
            message = "Exit recorded. No payment was required. Open the exit barrier manually.";
        }
    } else {
        message = "Payment required. Collect the payment and confirm it before opening the exit barrier.";
    }

    return true;
}

bool ParkingSystem::confirmPayment(long long transactionId, PaymentMethod paymentMethod,
                                   ParkingTransaction& transaction, std::string& message) {
    if (paymentMethod == PaymentMethod::NotSelected) {
        message = "Select a payment method before confirming payment.";
        return false;
    }
    if (paymentMethod != PaymentMethod::Cash) {
        message = "Only manual cash collection is supported right now. Card and M-Pesa integration is coming soon.";
        return false;
    }

    ParkingTransaction* record = findTransaction(transactionId);
    if (!record) {
        message = "Transaction not found.";
        return false;
    }
    if (record->paymentStatus == PaymentStatus::Paid) {
        transaction = *record;
        message = "Payment has already been confirmed for this transaction.";
        return false;
    }

    record->paymentStatus = PaymentStatus::Paid;
    record->paymentMethod = paymentMethod;

    ParkingSlot* slot = findVehicleSlot(record->vehicleNumber);
    if (!slot) {
        record->paymentStatus = PaymentStatus::Pending;
        record->paymentMethod = PaymentMethod::NotSelected;
        message = "The vehicle slot could not be found, so the payment was not completed.";
        return false;
    }

    slot->status = SlotStatus::Available;
    slot->vehicleNumber.clear();
    slot->entryTime = 0;

    if (!saveChanges()) {
        slot->status = SlotStatus::Occupied;
        slot->vehicleNumber = record->vehicleNumber;
        slot->entryTime = record->entryTime;
        record->paymentStatus = PaymentStatus::Pending;
        record->paymentMethod = PaymentMethod::NotSelected;
        message = database.error();
        return false;
    }

    transaction = *record;

    if (barriers.automaticMode) {
        automaticOpenExitBarrier();
        message = "Payment confirmed. Exit barrier opened automatically.";
    } else {
        message = "Payment confirmed. Open the exit barrier manually.";
    }
    return true;
}

bool ParkingSystem::findVehicle(const std::string& rawVehicleNumber, ParkingSlot& result) const {
    std::string vehicleNumber = normalizeVehicleNumber(rawVehicleNumber);
    for (const auto& slot : slots) {
        if (slot.status == SlotStatus::Occupied && slot.vehicleNumber == vehicleNumber) {
            result = slot;
            return true;
        }
    }
    return false;
}

bool ParkingSystem::addBlacklist(const std::string& rawVehicleNumber, std::string& message) {
    std::string number = normalizeVehicleNumber(rawVehicleNumber);
    if (number.empty() || number.size() >= MAX_VEHICLE_NUMBER) {
        message = "Enter a valid registration number.";
        return false;
    }
    if (isBlacklisted(number)) {
        message = "Vehicle is already blacklisted.";
        return false;
    }
    if (blacklist.size() >= MAX_BLACKLIST) {
        message = "Blacklist limit reached.";
        return false;
    }

    blacklist.push_back(number);
    if (!saveChanges()) {
        blacklist.pop_back();
        message = database.error();
        return false;
    }

    message = "Vehicle added to the blacklist.";
    return true;
}

bool ParkingSystem::removeBlacklist(const std::string& rawVehicleNumber, std::string& message) {
    std::string number = normalizeVehicleNumber(rawVehicleNumber);
    auto it = std::find(blacklist.begin(), blacklist.end(), number);
    if (it == blacklist.end()) {
        message = "Vehicle is not on the blacklist.";
        return false;
    }

    blacklist.erase(it);
    if (!saveChanges()) {
        message = database.error();
        return false;
    }

    message = "Vehicle removed from the blacklist.";
    return true;
}

bool ParkingSystem::isBlacklisted(const std::string& rawVehicleNumber) const {
    std::string number = normalizeVehicleNumber(rawVehicleNumber);
    return std::find(blacklist.begin(), blacklist.end(), number) != blacklist.end();
}

bool ParkingSystem::updatePricing(const PricingSettings& settings, std::string& message) {
    if (settings.freeLimitMinutes < 0 ||
        settings.firstLimitMinutes <= settings.freeLimitMinutes ||
        settings.secondLimitMinutes <= settings.firstLimitMinutes ||
        settings.thirdLimitMinutes <= settings.secondLimitMinutes ||
        settings.firstFee < 0 || settings.secondFee < 0 ||
        settings.thirdFee < 0 || settings.maximumFee < 0) {
        message = "Pricing limits and fees are not valid.";
        return false;
    }

    pricing = settings;
    if (!saveChanges()) {
        message = database.error();
        return false;
    }

    message = "Parking rates updated successfully.";
    return true;
}

bool ParkingSystem::updateTransaction(long long id, int amount, PaymentStatus paymentStatus,
                                      PaymentMethod paymentMethod, std::string& message) {
    ParkingTransaction* record = findTransaction(id);
    if (!record) {
        message = "Transaction not found.";
        return false;
    }
    if (amount < 0) {
        message = "Amount cannot be negative.";
        return false;
    }

    // Payment completion has a separate action so the slot and barrier stay
    // consistent with the payment record.
    if (record->paymentStatus == PaymentStatus::Pending && paymentStatus == PaymentStatus::Paid) {
        message = "Use the payment confirmation action to complete a pending payment.";
        return false;
    }
    if (record->paymentStatus == PaymentStatus::Paid && paymentStatus == PaymentStatus::Pending) {
        message = "A completed payment cannot be changed back to pending.";
        return false;
    }

    record->amount = amount;
    record->paymentStatus = paymentStatus;
    record->paymentMethod = paymentMethod;

    if (!saveChanges()) {
        message = database.error();
        return false;
    }

    message = "Transaction updated successfully.";
    return true;
}

const std::vector<ParkingSlot>& ParkingSystem::getSlots() const { return slots; }
const std::vector<ParkingTransaction>& ParkingSystem::getTransactions() const { return transactions; }
const std::vector<std::string>& ParkingSystem::getBlacklist() const { return blacklist; }
const PricingSettings& ParkingSystem::getPricing() const { return pricing; }

const BarrierState& ParkingSystem::getBarriers() const {
    updateAutomaticBarriers();
    return barriers;
}

void ParkingSystem::updateAutomaticBarriers() const {
    if (!barriers.automaticMode) return;

    const std::time_t now = std::time(nullptr);
    if (barriers.entranceOpen && entranceOpenedAt > 0 &&
        std::difftime(now, entranceOpenedAt) >= BARRIER_OPEN_SECONDS) {
        barriers.entranceOpen = false;
        entranceOpenedAt = 0;
    }
    if (barriers.exitOpen && exitOpenedAt > 0 &&
        std::difftime(now, exitOpenedAt) >= BARRIER_OPEN_SECONDS) {
        barriers.exitOpen = false;
        exitOpenedAt = 0;
    }
}

void ParkingSystem::automaticOpenEntranceBarrier() {
    barriers.entranceOpen = true;
    entranceOpenedAt = std::time(nullptr);
}

void ParkingSystem::automaticOpenExitBarrier() {
    barriers.exitOpen = true;
    exitOpenedAt = std::time(nullptr);
}

void ParkingSystem::openEntranceBarrier() {
    barriers.entranceOpen = true;
    entranceOpenedAt = 0;
}

void ParkingSystem::closeEntranceBarrier() {
    barriers.entranceOpen = false;
    entranceOpenedAt = 0;
}

void ParkingSystem::openExitBarrier() {
    barriers.exitOpen = true;
    exitOpenedAt = 0;
}

void ParkingSystem::closeExitBarrier() {
    barriers.exitOpen = false;
    exitOpenedAt = 0;
}

void ParkingSystem::setAutomaticBarrierMode(bool enabled) {
    barriers.automaticMode = enabled;
    if (!enabled) {
        // Manual mode should not leave an old automatic timer active.
        entranceOpenedAt = 0;
        exitOpenedAt = 0;
    }
}

int ParkingSystem::availableSlots() const {
    return static_cast<int>(std::count_if(slots.begin(), slots.end(), [](const ParkingSlot& s) {
        return s.status == SlotStatus::Available;
    }));
}

int ParkingSystem::occupiedSlots() const {
    return TOTAL_SLOTS - availableSlots();
}

std::string ParkingSystem::slotCode(const ParkingSlot& slot) const {
    std::string code = std::to_string(slot.floor);
    code += slot.wing == 0 ? 'A' : 'B';
    if (slot.number < 10) code += '0';
    code += std::to_string(slot.number);
    return code;
}

std::string ParkingSystem::lastError() const {
    return database.error();
}
