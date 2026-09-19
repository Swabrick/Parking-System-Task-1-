#include "parking.h"
#include "config.h"
#include "utils.h"
#include <algorithm>
#include <cctype>
#include <ctime>

ParkingSystem::ParkingSystem(Database& database) : database(database) {}

void ParkingSystem::buildSlots() {
    slots.clear();
    slots.reserve(TOTAL_SLOTS);
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
    if (!database.load(slots, transactions)) return false;
    if (slots.size() != TOTAL_SLOTS) {
        buildSlots();
        if (!database.save(slots, transactions)) return false;
    }
    return true;
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

bool ParkingSystem::enterVehicle(const std::string& rawVehicleNumber, std::string& message) {
    std::string vehicleNumber = rawVehicleNumber;
    trimAndUpper(vehicleNumber);

    if (vehicleNumber.empty() || vehicleNumber.size() >= MAX_VEHICLE_NUMBER) {
        message = "Enter a valid vehicle registration number.";
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

    if (!database.save(slots, transactions)) {
        slot->status = SlotStatus::Available;
        slot->vehicleNumber.clear();
        slot->entryTime = 0;
        message = database.error();
        return false;
    }

    message = "Vehicle assigned to slot " + slotCode(*slot) + ".";
    return true;
}

int ParkingSystem::calculateFee(long long durationMinutes) const {
    if (durationMinutes <= 30) return 0;
    if (durationMinutes <= 120) return 50;
    if (durationMinutes <= 240) return 100;
    if (durationMinutes <= 360) return 300;
    return 500;
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

    std::time_t exitTime = std::time(nullptr);
    long long seconds = static_cast<long long>(std::difftime(exitTime, slot->entryTime));
    long long minutes = (seconds + 59) / 60;

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

    transactions.push_back(transaction);

    slot->status = SlotStatus::Available;
    slot->vehicleNumber.clear();
    slot->entryTime = 0;

    if (!database.save(slots, transactions)) {
        transactions.pop_back();
        message = database.error();
        return false;
    }

    message = "Vehicle exit recorded successfully.";
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

const std::vector<ParkingSlot>& ParkingSystem::getSlots() const { return slots; }
const std::vector<ParkingTransaction>& ParkingSystem::getTransactions() const { return transactions; }
int ParkingSystem::availableSlots() const { return static_cast<int>(std::count_if(slots.begin(), slots.end(), [](const ParkingSlot& s) { return s.status == SlotStatus::Available; })); }
int ParkingSystem::occupiedSlots() const { return TOTAL_SLOTS - availableSlots(); }

std::string ParkingSystem::slotCode(const ParkingSlot& slot) const {
    std::string code = std::to_string(slot.floor);
    code += slot.wing == 0 ? 'A' : 'B';
    if (slot.number < 10) code += '0';
    code += std::to_string(slot.number);
    return code;
}

std::string ParkingSystem::lastError() const { return database.error(); }
