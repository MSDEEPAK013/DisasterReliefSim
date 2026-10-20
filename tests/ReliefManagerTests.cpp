
#include "ReliefManager.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

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

    std::cout << "All ReliefManager routing tests passed!\n";
    return 0;
}
