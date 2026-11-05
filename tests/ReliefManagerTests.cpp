
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

    // Vehicles with travel time: cargo arrives later, units on the road are not
    // sent twice, and the vehicle is busy until it is back home.
    {
        ReliefManager two;
        two.setTravelSpeed(10.0);                         // road cost 10 = one step each way
        two.addZone({1, "Zone A", 5, 1000, 80});
        ResourceCenter hub(101, "Hub");
        hub.addStock(DEFAULT_SUPPLY_ID, 500);
        two.addCenter(hub);
        two.addVehicle(Vehicle(1, 50, 101));
        two.addVehicle(Vehicle(2, 50, 101));
        two.addRoad(101, 1, 10.0);

        std::ostringstream quietTwo;
        std::streambuf* keep = std::cout.rdbuf(quietTwo.rdbuf());

        two.runStep();                                    // step 1: both vehicles leave (50 + 30)
        assert(two.findZone(1)->unmetDemand == 80);       // nothing has arrived yet
        assert(two.findCenter(101)->availableStock(DEFAULT_SUPPLY_ID) == 420);
        assert(two.findVehicle(1)->isOnTrip() && two.findVehicle(2)->isOnTrip());

        assert(two.runStep());                            // step 2: cargo arrives, vehicles drive home
        assert(two.findZone(1)->unmetDemand == 0);
        assert(two.findCenter(101)->availableStock(DEFAULT_SUPPLY_ID) == 420);   // no second dispatch

        assert(!two.runStep());                           // step 3: back home, nothing left to do
        assert(!two.findVehicle(1)->isOnTrip() && !two.findVehicle(2)->isOnTrip());

        // One vehicle has to make two round trips for 80 units.
        ReliefManager one;
        one.setTravelSpeed(10.0);
        one.addZone({1, "Zone A", 5, 1000, 80});
        ResourceCenter hub2(101, "Hub");
        hub2.addStock(DEFAULT_SUPPLY_ID, 500);
        one.addCenter(hub2);
        one.addVehicle(Vehicle(1, 50, 101));
        one.addRoad(101, 1, 10.0);

        one.runStep();                                    // step 1: leaves with 50
        assert(one.findZone(1)->unmetDemand == 80);
        one.runStep();                                    // step 2: 50 delivered, still driving home
        assert(one.findZone(1)->unmetDemand == 30);
        assert(one.findVehicle(1)->isOnTrip());
        assert(one.findCenter(101)->availableStock(DEFAULT_SUPPLY_ID) == 450);   // busy: nothing new sent
        one.runStep();                                    // step 3: back home, leaves with 30
        assert(one.findCenter(101)->availableStock(DEFAULT_SUPPLY_ID) == 420);
        one.runStep();                                    // step 4: last 30 delivered
        assert(!one.hasUnmetDemand());

        std::cout.rdbuf(keep);

        // A negative speed is rejected.
        bool badSpeed = false;
        try { one.setTravelSpeed(-1.0); } catch (const std::invalid_argument&) { badSpeed = true; }
        assert(badSpeed);
    }
    std::cout << "All ReliefManager routing tests passed!\n";
    return 0;
}
