#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include "models.h"

class Database {
public:
    explicit Database(const std::string& path);

    bool open();
    bool initialize();
    bool load(std::vector<ParkingSlot>& slots, std::vector<ParkingTransaction>& transactions);
    bool save(const std::vector<ParkingSlot>& slots,
              const std::vector<ParkingTransaction>& transactions);
    const std::string& error() const;

private:
    std::string path;
    std::string lastError;
};

#endif
