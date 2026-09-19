#include "database.h"
#include "config.h"
#include <filesystem>
#include <fstream>
#include <cstdint>

namespace {
    const char MAGIC[] = "SPDB1";

    void writeString(std::ofstream& out, const std::string& value) {
        std::uint32_t size = static_cast<std::uint32_t>(value.size());
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
        out.write(value.data(), size);
    }

    bool readString(std::ifstream& in, std::string& value) {
        std::uint32_t size = 0;
        if (!in.read(reinterpret_cast<char*>(&size), sizeof(size))) return false;
        if (size > 10000) return false;
        value.resize(size);
        return static_cast<bool>(in.read(value.data(), size));
    }
}

Database::Database(const std::string& path) : path(path) {}

bool Database::open() {
    try {
        std::filesystem::path file(path);
        if (!file.parent_path().empty()) std::filesystem::create_directories(file.parent_path());
        if (!std::filesystem::exists(file)) {
            std::ofstream create(file, std::ios::binary);
            if (!create) {
                lastError = "Could not create the local database file.";
                return false;
            }
        }
        return true;
    } catch (...) {
        lastError = "Could not access the local database folder.";
        return false;
    }
}

bool Database::initialize() {
    return open();
}

bool Database::load(std::vector<ParkingSlot>& slots, std::vector<ParkingTransaction>& transactions) {
    slots.clear();
    transactions.clear();

    std::ifstream in(path, std::ios::binary);
    if (!in || in.peek() == std::ifstream::traits_type::eof()) return true;

    char magic[sizeof(MAGIC)]{};
    if (!in.read(magic, sizeof(MAGIC) - 1) || std::string(magic, sizeof(MAGIC) - 1) != MAGIC) {
        lastError = "The local database file is invalid.";
        return false;
    }

    std::uint32_t slotCount = 0;
    std::uint32_t transactionCount = 0;
    if (!in.read(reinterpret_cast<char*>(&slotCount), sizeof(slotCount)) ||
        !in.read(reinterpret_cast<char*>(&transactionCount), sizeof(transactionCount))) {
        lastError = "The local database file is incomplete.";
        return false;
    }

    if (slotCount > TOTAL_SLOTS || transactionCount > MAX_TRANSACTIONS) {
        lastError = "The local database contains invalid record counts.";
        return false;
    }

    for (std::uint32_t i = 0; i < slotCount; ++i) {
        ParkingSlot slot;
        int status = 0;
        if (!in.read(reinterpret_cast<char*>(&slot.floor), sizeof(slot.floor)) ||
            !in.read(reinterpret_cast<char*>(&slot.wing), sizeof(slot.wing)) ||
            !in.read(reinterpret_cast<char*>(&slot.number), sizeof(slot.number)) ||
            !in.read(reinterpret_cast<char*>(&status), sizeof(status)) ||
            !in.read(reinterpret_cast<char*>(&slot.entryTime), sizeof(slot.entryTime)) ||
            !readString(in, slot.vehicleNumber)) {
            lastError = "The local database contains a damaged slot record.";
            return false;
        }
        slot.status = status == 1 ? SlotStatus::Occupied : SlotStatus::Available;
        slots.push_back(slot);
    }

    for (std::uint32_t i = 0; i < transactionCount; ++i) {
        ParkingTransaction tx;
        int status = 1;
        if (!in.read(reinterpret_cast<char*>(&tx.id), sizeof(tx.id)) ||
            !in.read(reinterpret_cast<char*>(&tx.floor), sizeof(tx.floor)) ||
            !in.read(reinterpret_cast<char*>(&tx.wing), sizeof(tx.wing)) ||
            !in.read(reinterpret_cast<char*>(&tx.slot), sizeof(tx.slot)) ||
            !in.read(reinterpret_cast<char*>(&tx.entryTime), sizeof(tx.entryTime)) ||
            !in.read(reinterpret_cast<char*>(&tx.exitTime), sizeof(tx.exitTime)) ||
            !in.read(reinterpret_cast<char*>(&tx.durationMinutes), sizeof(tx.durationMinutes)) ||
            !in.read(reinterpret_cast<char*>(&tx.amount), sizeof(tx.amount)) ||
            !in.read(reinterpret_cast<char*>(&status), sizeof(status)) ||
            !readString(in, tx.vehicleNumber)) {
            lastError = "The local database contains a damaged transaction record.";
            return false;
        }
        tx.status = status == 0 ? SessionStatus::Active : SessionStatus::Completed;
        transactions.push_back(tx);
    }

    return true;
}

bool Database::save(const std::vector<ParkingSlot>& slots,
                    const std::vector<ParkingTransaction>& transactions) {
    try {
        std::filesystem::path file(path);
        if (!file.parent_path().empty()) std::filesystem::create_directories(file.parent_path());

        std::string tempPath = path + ".tmp";
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            lastError = "Could not write to the local database.";
            return false;
        }

        out.write(MAGIC, sizeof(MAGIC) - 1);
        std::uint32_t slotCount = static_cast<std::uint32_t>(slots.size());
        std::uint32_t transactionCount = static_cast<std::uint32_t>(transactions.size());
        out.write(reinterpret_cast<const char*>(&slotCount), sizeof(slotCount));
        out.write(reinterpret_cast<const char*>(&transactionCount), sizeof(transactionCount));

        for (const auto& slot : slots) {
            int status = slot.status == SlotStatus::Occupied ? 1 : 0;
            out.write(reinterpret_cast<const char*>(&slot.floor), sizeof(slot.floor));
            out.write(reinterpret_cast<const char*>(&slot.wing), sizeof(slot.wing));
            out.write(reinterpret_cast<const char*>(&slot.number), sizeof(slot.number));
            out.write(reinterpret_cast<const char*>(&status), sizeof(status));
            out.write(reinterpret_cast<const char*>(&slot.entryTime), sizeof(slot.entryTime));
            writeString(out, slot.vehicleNumber);
        }

        for (const auto& tx : transactions) {
            int status = tx.status == SessionStatus::Completed ? 1 : 0;
            out.write(reinterpret_cast<const char*>(&tx.id), sizeof(tx.id));
            out.write(reinterpret_cast<const char*>(&tx.floor), sizeof(tx.floor));
            out.write(reinterpret_cast<const char*>(&tx.wing), sizeof(tx.wing));
            out.write(reinterpret_cast<const char*>(&tx.slot), sizeof(tx.slot));
            out.write(reinterpret_cast<const char*>(&tx.entryTime), sizeof(tx.entryTime));
            out.write(reinterpret_cast<const char*>(&tx.exitTime), sizeof(tx.exitTime));
            out.write(reinterpret_cast<const char*>(&tx.durationMinutes), sizeof(tx.durationMinutes));
            out.write(reinterpret_cast<const char*>(&tx.amount), sizeof(tx.amount));
            out.write(reinterpret_cast<const char*>(&status), sizeof(status));
            writeString(out, tx.vehicleNumber);
        }

        out.close();
        if (!out) {
            lastError = "Could not finish writing the local database.";
            return false;
        }

        std::filesystem::rename(tempPath, path + ".bak");
        std::error_code ec;
        std::filesystem::remove(path, ec);
        std::filesystem::rename(path + ".bak", path, ec);
        if (ec) {
            lastError = "Could not replace the local database file.";
            return false;
        }
        return true;
    } catch (...) {
        lastError = "An error occurred while saving the local database.";
        return false;
    }
}

const std::string& Database::error() const {
    return lastError;
}
