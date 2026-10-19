#pragma once

#include "DisasterZone.h"
#include "PriorityManager.h"
#include "ResourceCenter.h"
#include "Vehicle.h"

#include <unordered_map>
#include <vector>

// Owns all simulation objects and runs the relief workflow step by step.
class ReliefManager {
public:
    explicit ReliefManager(PriorityWeights weights = PriorityWeights());

    // Registration. Each throws std::invalid_argument on a bad or duplicate id.
    void addZone(const DisasterZone& zone);
    void addCenter(const ResourceCenter& center);
    void addVehicle(const Vehicle& vehicle);

    // Lookup by id (hash table). Returns nullptr if the id is unknown.
    const DisasterZone* findZone(int id) const;
    const ResourceCenter* findCenter(int id) const;
    const Vehicle* findVehicle(int id) const;

    bool hasUnmetDemand() const;

    // Runs one simulation step. Returns false if there was nothing to do.
    bool runStep();

private:
    std::unordered_map<int, DisasterZone> zones;
    std::unordered_map<int, ResourceCenter> centers;
    std::unordered_map<int, Vehicle> vehicles;
    PriorityManager priority;
};