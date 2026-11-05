
#include "ReliefManager.h"
#include "Types.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <sstream>
#include <stdexcept>

int main() {
    ReliefManager manager;

    manager.addRoad(1, 2, 4.0);
    manager.addRoad(2, 3, 6.0);
    manager.addRoad(1, 3, 15.0);

    // Check shortest distance through ReliefManager.
    assert(manager.shortestDistance(1, 3) == 10.0);

    // Check shortest path through ReliefManager.
    std::vector<int> expectedPath = {1, 2, 3};
    assert(manager.shortestPath(1, 3) == expectedPath);

    // Block a road and check that routing changes.
    manager.setRoadBlocked(2, 3, true);

    assert(manager.shortestDistance(1, 3) == 15.0);

    expectedPath = {1, 3};
    assert(manager.shortestPath(1, 3) == expectedPath);

    // Check an unreachable destination.
    manager.setRoadBlocked(1, 3, true);

    assert(std::isinf(manager.shortestDistance(1, 3)));
    assert(manager.shortestPath(1, 3).empty());

    // Check BFS integration through ReliefManager.
ReliefManager bfsManager;

DisasterZone zone;
zone.id = 3;
zone.name = "Zone A";
zone.severity = 5;
zone.affectedPopulation = 100;
zone.unmetDemand = 50;
zone.nodeId = 3;

bfsManager.addZone(zone);
ResourceCenter centerA(1, "Center A", 1);
centerA.addStock(DEFAULT_SUPPLY_ID, 100);   // an empty center is no longer listed as a route source
bfsManager.addCenter(centerA);

bfsManager.addRoad(1, 2, 4.0);
bfsManager.addRoad(2, 3, 6.0);

std::ostringstream output;
std::streambuf* oldOutput = std::cout.rdbuf(output.rdbuf());

bfsManager.runStep();

std::cout.rdbuf(oldOutput);

assert(output.str().find("Reachability") != std::string::npos);
assert(output.str().find("Route:") != std::string::npos);
assert(output.str().find("Center A (1) -> Junction 2") != std::string::npos);

    // Dispatch changes the state: stock and demand both go down.
    ReliefManager dispatch;
    dispatch.addZone({1, "Zone A", 5, 1000, 80});
    ResourceCenter depot(101, "Depot");
    depot.addStock(DEFAULT_SUPPLY_ID, 200);
    dispatch.addCenter(depot);
    dispatch.addVehicle(Vehicle(1, 50, 101));
    dispatch.addRoad(101, 1, 5.0);

    std::ostringstream sink;
    std::streambuf* saved = std::cout.rdbuf(sink.rdbuf());
    bool first = dispatch.runStep();
    assert(dispatch.findZone(1)->unmetDemand == 30);        // 80 - 50 (vehicle capacity)
    assert(dispatch.findCenter(101)->availableStock(DEFAULT_SUPPLY_ID) == 150);
    bool second = dispatch.runStep();
    std::cout.rdbuf(saved);

    assert(first && second);
    assert(!dispatch.hasUnmetDemand());                     // 30 - 30
    assert(dispatch.findCenter(101)->availableStock(DEFAULT_SUPPLY_ID) == 120);

    // A step that dispatches nothing returns false, so a Simulation stops early.
    ReliefManager stuck;
    stuck.addZone({1, "Zone A", 5, 1000, 80});
    stuck.addCenter(ResourceCenter(101, "Empty Depot"));    // no stock
    stuck.addVehicle(Vehicle(1, 50, 101));
    stuck.addRoad(101, 1, 5.0);

    std::ostringstream quiet;
    std::streambuf* old = std::cout.rdbuf(quiet.rdbuf());
    bool progressed = stuck.runStep();
    std::cout.rdbuf(old);
    assert(!progressed);

// Check unreachable zone when a road is blocked.
bfsManager.setRoadBlocked(2, 3, true);

std::ostringstream blockedOutput;
oldOutput = std::cout.rdbuf(blockedOutput.rdbuf());

bfsManager.runStep();

std::cout.rdbuf(oldOutput);

assert(blockedOutput.str().find("Zone A: UNREACHABLE") != std::string::npos);
assert(blockedOutput.str().find("Route:") == std::string::npos);

    // Resource registry: kinds of supply are stored in a hash table by id.
    ReliefManager registry;
    assert(registry.findResource(101) == nullptr);

    registry.addResource(Resource(101, "Water", "litres"));
    registry.addResource(Resource(102, "Medicine", "boxes"));

    const Resource* water = registry.findResource(101);
    assert(water != nullptr);
    assert(water->name() == "Water" && water->unit() == "litres");
    assert(registry.findResource(102)->name() == "Medicine");
    assert(registry.findResource(103) == nullptr);

    // A duplicate id is rejected and the original stays as it was.
    bool rejectedDuplicate = false;
    try {
        registry.addResource(Resource(101, "Food"));
    } catch (const std::invalid_argument&) {
        rejectedDuplicate = true;
    }
    assert(rejectedDuplicate);
    assert(registry.findResource(101)->name() == "Water");

    std::cout << "All ReliefManager routing tests passed!\n";
    return 0;
}
