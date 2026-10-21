#include "ScenarioBuilder.h"

#include <random>
#include <stdexcept>
#include <string>

using namespace std;

namespace {
    const int SUPPLY_ID = 101;

    DisasterZone makeZone(int id, const string& name, int severity,
                          int population, int demand) {
        DisasterZone z;
        z.id = id;
        z.name = name;
        z.severity = severity;
        z.affectedPopulation = population;
        z.unmetDemand = demand;
        return z;
    }
}

namespace ScenarioBuilder {

void basic(ReliefManager& manager) {
    manager.addZone(makeZone(1, "North Sector", 5, 1200, 500));
    manager.addZone(makeZone(2, "River District", 4, 2000, 300));
    manager.addZone(makeZone(3, "East Village", 3, 800, 700));

    ResourceCenter warehouse(1, "Central Warehouse");
    warehouse.addStock(SUPPLY_ID, 2000);
    manager.addCenter(warehouse);

    manager.addVehicle(Vehicle(1, 100, 0));
    manager.addVehicle(Vehicle(2, 60, 0));
}

void insufficientSupply(ReliefManager& manager) {
    manager.addZone(makeZone(1, "North Sector", 5, 1200, 800));
    manager.addZone(makeZone(2, "River District", 4, 2000, 900));

    ResourceCenter depot(1, "Small Depot");
    depot.addStock(SUPPLY_ID, 100);   // far less than the 1700 needed
    manager.addCenter(depot);

    manager.addVehicle(Vehicle(1, 50, 0));
}

void noVehicles(ReliefManager& manager) {
    manager.addZone(makeZone(1, "North Sector", 5, 1200, 500));

    ResourceCenter warehouse(1, "Central Warehouse");
    warehouse.addStock(SUPPLY_ID, 1000);
    manager.addCenter(warehouse);
    // deliberately no vehicles
}

void equalPriority(ReliefManager& manager) {
    manager.addZone(makeZone(2, "Zone B", 4, 1000, 400));
    manager.addZone(makeZone(1, "Zone A", 4, 1000, 400));  // same numbers, lower id

    ResourceCenter warehouse(1, "Central Warehouse");
    warehouse.addStock(SUPPLY_ID, 1000);
    manager.addCenter(warehouse);

    manager.addVehicle(Vehicle(1, 100, 0));
}

void random(ReliefManager& manager, unsigned int seed,
            int zoneCount, int centerCount, int vehicleCount) {
    if (zoneCount < 0 || centerCount < 0 || vehicleCount < 0) {
        throw invalid_argument("Scenario sizes cannot be negative.");
    }

    mt19937 rng(seed);   // deterministic engine
    // Plain modulo is used instead of std::uniform_int_distribution because
    // the standard does not guarantee identical distribution output across
    // compilers, and reproducibility is a project requirement.

    for (int i = 1; i <= zoneCount; i++) {
        int severity = 1 + (int)(rng() % 5);
        int population = 100 + (int)(rng() % 5000);
        int demand = 50 + (int)(rng() % 1000);
        manager.addZone(makeZone(i, "Zone " + to_string(i),
                                 severity, population, demand));
    }

    for (int i = 1; i <= centerCount; i++) {
        ResourceCenter center(i, "Center " + to_string(i));
        center.addStock(SUPPLY_ID, 200 + (int)(rng() % 800));
        manager.addCenter(center);
    }

    for (int i = 1; i <= vehicleCount; i++) {
        manager.addVehicle(Vehicle(i, 20 + (int)(rng() % 80), 0));
    }
}

}  