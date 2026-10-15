#pragma once
#include <string>

struct DisasterZone {
    int id{};
    std::string name;
    int severity{}; // 1 (low) to 5 (critical)
    int affectedPopulation{};
    int unmetDemand{};

    bool isValid() const {
        return id >= 0 && !name.empty() &&
               severity >= 1 && severity <= 5 &&
               affectedPopulation >= 0 && unmetDemand >= 0;
    }
};
