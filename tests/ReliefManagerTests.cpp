
#include "ReliefManager.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <sstream>

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
bfsManager.addCenter(ResourceCenter(1, "Center A", 1));

bfsManager.addRoad(1, 2, 4.0);
bfsManager.addRoad(2, 3, 6.0);

std::ostringstream output;
std::streambuf* oldOutput = std::cout.rdbuf(output.rdbuf());

bfsManager.runStep();

std::cout.rdbuf(oldOutput);

assert(output.str().find("Reachability") != std::string::npos);
assert(output.str().find("Route:") != std::string::npos);
assert(output.str().find("Center A (1) -> Junction 2") != std::string::npos);

// Check unreachable zone when a road is blocked.
bfsManager.setRoadBlocked(2, 3, true);

std::ostringstream blockedOutput;
oldOutput = std::cout.rdbuf(blockedOutput.rdbuf());

bfsManager.runStep();

std::cout.rdbuf(oldOutput);

assert(blockedOutput.str().find("Zone A: UNREACHABLE") != std::string::npos);
assert(blockedOutput.str().find("Route:") == std::string::npos);

    std::cout << "All ReliefManager routing tests passed!\n";
    return 0;
}
