#include "database.h"
#include "config.h"
#include <filesystem>
#include <fstream>
#include <cstdint>

namespace {
    const char MAGIC_V1[] = "SPDB1";
    const char MAGIC_V2[] = "SPDB2";

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

    // SPDB1 did not contain the newer blacklist, pricing and payment fields.
    // Older records are still accepted so existing local data is not lost.
    bool readV1(std::ifstream& in, std::vector<ParkingSlot>& slots,
                std::vector<ParkingTransaction>& transactions) {
        std::uint32_t slotCount = 0;
        std::uint32_t transactionCount = 0;
        if (!in.read(reinterpret_cast<char*>(&slotCount), sizeof(slotCount)) ||
            !in.read(reinterpret_cast<char*>(&transactionCount), sizeof(transactionCount))) return false;
        if (slotCount > TOTAL_SLOTS || transactionCount > MAX_TRANSACTIONS) return false;

        for (std::uint32_t i = 0; i < slotCount; ++i) {
            ParkingSlot slot;
            int status = 0;
            if (!in.read(reinterpret_cast<char*>(&slot.floor), sizeof(slot.floor)) ||
                !in.read(reinterpret_cast<char*>(&slot.wing), sizeof(slot.wing)) ||
                !in.read(reinterpret_cast<char*>(&slot.number), sizeof(slot.number)) ||
                !in.read(reinterpret_cast<char*>(&status), sizeof(status)) ||
                !in.read(reinterpret_cast<char*>(&slot.entryTime), sizeof(slot.entryTime)) ||
                !readString(in, slot.vehicleNumber)) return false;
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
                !readString(in, tx.vehicleNumber)) return false;
            tx.status = status == 0 ? SessionStatus::Active : SessionStatus::Completed;
            tx.paymentStatus = PaymentStatus::Paid;
            tx.paymentMethod = PaymentMethod::Cash;
            transactions.push_back(tx);
        }
        return true;
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

bool Database::initialize() { return open(); }

bool Database::load(std::vector<ParkingSlot>& slots,
                    std::vector<ParkingTransaction>& transactions,
                    std::vector<std::string>& blacklist,
                    PricingSettings& pricing) {
    slots.clear();
    transactions.clear();
    blacklist.clear();
    pricing = PricingSettings{};

    std::ifstream in(path, std::ios::binary);
    if (!in || in.peek() == std::ifstream::traits_type::eof()) return true;

    char magic[6]{};
    if (!in.read(magic, 5)) {
        lastError = "The local database file is incomplete.";
        return false;
    }

    if (std::string(magic, 5) == MAGIC_V1) {
        if (!readV1(in, slots, transactions)) {
            lastError = "The old local database could not be read.";
            return false;
        }
        return true;
    }

    if (std::string(magic, 5) != MAGIC_V2) {
        lastError = "The local database file is invalid.";
        return false;
    }

    std::uint32_t slotCount = 0;
    std::uint32_t transactionCount = 0;
    std::uint32_t blacklistCount = 0;

    if (!in.read(reinterpret_cast<char*>(&slotCount), sizeof(slotCount)) ||
        !in.read(reinterpret_cast<char*>(&transactionCount), sizeof(transactionCount)) ||
        !in.read(reinterpret_cast<char*>(&blacklistCount), sizeof(blacklistCount))) {
        lastError = "The local database file is incomplete.";
        return false;
    }

    if (slotCount > TOTAL_SLOTS || transactionCount > MAX_TRANSACTIONS || blacklistCount > MAX_BLACKLIST) {
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
        int sessionStatus = 1;
        int paymentStatus = 0;
        int paymentMethod = 0;
        if (!in.read(reinterpret_cast<char*>(&tx.id), sizeof(tx.id)) ||
            !in.read(reinterpret_cast<char*>(&tx.floor), sizeof(tx.floor)) ||
            !in.read(reinterpret_cast<char*>(&tx.wing), sizeof(tx.wing)) ||
            !in.read(reinterpret_cast<char*>(&tx.slot), sizeof(tx.slot)) ||
            !in.read(reinterpret_cast<char*>(&tx.entryTime), sizeof(tx.entryTime)) ||
            !in.read(reinterpret_cast<char*>(&tx.exitTime), sizeof(tx.exitTime)) ||
            !in.read(reinterpret_cast<char*>(&tx.durationMinutes), sizeof(tx.durationMinutes)) ||
            !in.read(reinterpret_cast<char*>(&tx.amount), sizeof(tx.amount)) ||
            !in.read(reinterpret_cast<char*>(&sessionStatus), sizeof(sessionStatus)) ||
            !in.read(reinterpret_cast<char*>(&paymentStatus), sizeof(paymentStatus)) ||
            !in.read(reinterpret_cast<char*>(&paymentMethod), sizeof(paymentMethod)) ||
            !readString(in, tx.vehicleNumber)) {
            lastError = "The local database contains a damaged transaction record.";
            return false;
        }
        tx.status = sessionStatus == 0 ? SessionStatus::Active : SessionStatus::Completed;
        tx.paymentStatus = paymentStatus == 1 ? PaymentStatus::Paid : PaymentStatus::Pending;
        tx.paymentMethod = static_cast<PaymentMethod>(paymentMethod);
        transactions.push_back(tx);
    }

    for (std::uint32_t i = 0; i < blacklistCount; ++i) {
        std::string number;
        if (!readString(in, number)) {
            lastError = "The local database contains a damaged blacklist record.";
            return false;
        }
        blacklist.push_back(number);
    }

    if (!in.read(reinterpret_cast<char*>(&pricing.freeLimitMinutes), sizeof(pricing.freeLimitMinutes)) ||
        !in.read(reinterpret_cast<char*>(&pricing.firstLimitMinutes), sizeof(pricing.firstLimitMinutes)) ||
        !in.read(reinterpret_cast<char*>(&pricing.secondLimitMinutes), sizeof(pricing.secondLimitMinutes)) ||
        !in.read(reinterpret_cast<char*>(&pricing.thirdLimitMinutes), sizeof(pricing.thirdLimitMinutes)) ||
        !in.read(reinterpret_cast<char*>(&pricing.firstFee), sizeof(pricing.firstFee)) ||
        !in.read(reinterpret_cast<char*>(&pricing.secondFee), sizeof(pricing.secondFee)) ||
        !in.read(reinterpret_cast<char*>(&pricing.thirdFee), sizeof(pricing.thirdFee)) ||
        !in.read(reinterpret_cast<char*>(&pricing.maximumFee), sizeof(pricing.maximumFee))) {
        lastError = "The local database is missing pricing settings.";
        return false;
    }

    return true;
}

bool Database::save(const std::vector<ParkingSlot>& slots,
                    const std::vector<ParkingTransaction>& transactions,
                    const std::vector<std::string>& blacklist,
                    const PricingSettings& pricing) {
    try {
        std::filesystem::path file(path);
        if (!file.parent_path().empty()) std::filesystem::create_directories(file.parent_path());

        const std::string tempPath = path + ".tmp";
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            lastError = "Could not write to the local database.";
            return false;
        }

        out.write(MAGIC_V2, 5);
        std::uint32_t slotCount = static_cast<std::uint32_t>(slots.size());
        std::uint32_t transactionCount = static_cast<std::uint32_t>(transactions.size());
        std::uint32_t blacklistCount = static_cast<std::uint32_t>(blacklist.size());
        out.write(reinterpret_cast<const char*>(&slotCount), sizeof(slotCount));
        out.write(reinterpret_cast<const char*>(&transactionCount), sizeof(transactionCount));
        out.write(reinterpret_cast<const char*>(&blacklistCount), sizeof(blacklistCount));

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
            int sessionStatus = tx.status == SessionStatus::Completed ? 1 : 0;
            int paymentStatus = tx.paymentStatus == PaymentStatus::Paid ? 1 : 0;
            int paymentMethod = static_cast<int>(tx.paymentMethod);
            out.write(reinterpret_cast<const char*>(&tx.id), sizeof(tx.id));
            out.write(reinterpret_cast<const char*>(&tx.floor), sizeof(tx.floor));
            out.write(reinterpret_cast<const char*>(&tx.wing), sizeof(tx.wing));
            out.write(reinterpret_cast<const char*>(&tx.slot), sizeof(tx.slot));
            out.write(reinterpret_cast<const char*>(&tx.entryTime), sizeof(tx.entryTime));
            out.write(reinterpret_cast<const char*>(&tx.exitTime), sizeof(tx.exitTime));
            out.write(reinterpret_cast<const char*>(&tx.durationMinutes), sizeof(tx.durationMinutes));
            out.write(reinterpret_cast<const char*>(&tx.amount), sizeof(tx.amount));
            out.write(reinterpret_cast<const char*>(&sessionStatus), sizeof(sessionStatus));
            out.write(reinterpret_cast<const char*>(&paymentStatus), sizeof(paymentStatus));
            out.write(reinterpret_cast<const char*>(&paymentMethod), sizeof(paymentMethod));
            writeString(out, tx.vehicleNumber);
        }

        for (const auto& number : blacklist) writeString(out, number);

        out.write(reinterpret_cast<const char*>(&pricing.freeLimitMinutes), sizeof(pricing.freeLimitMinutes));
        out.write(reinterpret_cast<const char*>(&pricing.firstLimitMinutes), sizeof(pricing.firstLimitMinutes));
        out.write(reinterpret_cast<const char*>(&pricing.secondLimitMinutes), sizeof(pricing.secondLimitMinutes));
        out.write(reinterpret_cast<const char*>(&pricing.thirdLimitMinutes), sizeof(pricing.thirdLimitMinutes));
        out.write(reinterpret_cast<const char*>(&pricing.firstFee), sizeof(pricing.firstFee));
        out.write(reinterpret_cast<const char*>(&pricing.secondFee), sizeof(pricing.secondFee));
        out.write(reinterpret_cast<const char*>(&pricing.thirdFee), sizeof(pricing.thirdFee));
        out.write(reinterpret_cast<const char*>(&pricing.maximumFee), sizeof(pricing.maximumFee));

        out.close();
        if (!out) {
            lastError = "The local database could not be written completely.";
            std::filesystem::remove(tempPath);
            return false;
        }

        // Replace the old file only after the new file has been written fully.
        std::filesystem::remove(path);
        std::filesystem::rename(tempPath, path);
        return true;
    } catch (...) {
        lastError = "Could not save the local database.";
        try { std::filesystem::remove(path + ".tmp"); } catch (...) {}
        return false;
    }
}

const std::string& Database::error() const { return lastError; }
