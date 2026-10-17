#pragma once
#include <string>

struct DisasterZone {
    int id = 0;
    std::string name;
    int severity = 0;            // 1 (low) to 5 (critical)
    int affectedPopulation = 0;
    int unmetDemand = 0;

    bool isValid() const {
        return id >= 0 && !name.empty() &&
               severity >= 1 && severity <= 5 &&
               affectedPopulation >= 0 && unmetDemand >= 0;
    }
};