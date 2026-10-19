#pragma once
#include <string>
using namespace std;

struct DisasterZone {
    int id = 0;
    string name;
    int severity = 0;            // 1 (low) to 5 (critical)
    int affectedPopulation = 0;
    int unmetDemand = 0;
    int nodeId = -1;

    bool isValid() const {
        if (id < 0) return false;
        if (name.empty()) return false;
        if (severity < 1 || severity > 5) return false;
        if (affectedPopulation < 0 || unmetDemand < 0) return false;
        return true;
    }
};