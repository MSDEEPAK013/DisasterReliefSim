#pragma once

#include "Graph.h"
#include "PriorityManager.h"
#include "ResourceCenter.h"
#include "Types.h"
#include "Vehicle.h"

#include <string>
#include <unordered_map>
#include <vector>

// A zone that still has unmet demand after planning, and why.
struct Shortfall {
    int zoneId = 0;
    int unserved = 0;        // units that could not be covered this step
    std::string reason;
};

// Everything the allocator decided for one simulation step.
struct AllocationPlan {
    std::vector<DispatchOrder> orders;
    std::vector<Shortfall> shortfalls;
};

class Allocator {
public:
    // zones is taken by value because the heap is emptied while planning.
    static AllocationPlan plan(ZoneQueue zones,
                               const std::unordered_map<int, ResourceCenter>& centers,
                               const std::unordered_map<int, Vehicle>& vehicles,
                               const Graph& graph,
                               int resourceId);
};