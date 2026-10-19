#pragma once
#include <vector>

// A path through the road network, produced by the graph module.
struct Route {
    bool found = false;
    double cost = 0.0;
    std::vector<int> path; // node ids from center to zone, in order
};

// One delivery decision, produced by the allocator.
struct DispatchOrder {
    int zoneId = 0;
    int centerId = 0;
    int vehicleId = 0;
    int resourceId = 0;
    int quantity = 0;
    Route route;
};