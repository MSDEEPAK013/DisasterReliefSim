#include "Graph.h"
#include <cassert>
#include <iostream>
#include <cmath>
#include <limits>

int main() {
    Graph graph;

    graph.addLocation(1);
    graph.addLocation(2);

    graph.addRoad(1, 2, 5.0);

    assert(!graph.isRoadBlocked(1, 2));
    assert(!graph.isRoadBlocked(2, 1));

    graph.setRoadBlocked(1, 2, true);

    assert(graph.isRoadBlocked(1, 2));
    assert(graph.isRoadBlocked(2, 1));

    graph.setRoadBlocked(1, 2, false);

    assert(!graph.isRoadBlocked(1, 2));
    assert(!graph.isRoadBlocked(2, 1));

    Graph routeGraph;

    routeGraph.addLocation(1);
    routeGraph.addLocation(2);
    routeGraph.addLocation(3);
    routeGraph.addLocation(4);

    routeGraph.addRoad(1, 2, 2.0);
    routeGraph.addRoad(2, 4, 3.0);
    routeGraph.addRoad(1, 3, 1.0);
    routeGraph.addRoad(3, 4, 10.0);

    assert(routeGraph.shortestDistance(1, 4) == 5.0);

    std::vector<int> expectedPath = {1, 2, 4};
    assert(routeGraph.shortestPath(1, 4) == expectedPath);

    routeGraph.setRoadBlocked(2, 4, true);

    assert(routeGraph.shortestDistance(1, 4) == 11);

    std::vector<int> alternativePath = {1, 3, 4};
    assert(routeGraph.shortestPath(1, 4) == alternativePath);

    // Destination is unreachable after blocking the remaining route
    routeGraph.setRoadBlocked(1, 3, true);

    assert(routeGraph.shortestPath(1, 4).empty());
    assert(std::isinf(routeGraph.shortestDistance(1, 4)));

// A location should have distance 0 from itself
    assert(routeGraph.shortestDistance(1, 1) == 0.0);

    std::vector<int> sameLocation = {1};
    assert(routeGraph.shortestPath(1, 1) == sameLocation);

// Unknown locations should be treated as unreachable
    assert(routeGraph.shortestPath(1, 99).empty());
    assert(std::isinf(routeGraph.shortestDistance(1, 99)));

    Graph invalidCostGraph;
    invalidCostGraph.addLocation(1);
    invalidCostGraph.addLocation(2);

    invalidCostGraph.addRoad(1, 2, -5.0);

    assert(std::isinf(invalidCostGraph.shortestDistance(1, 2)));
    assert(invalidCostGraph.shortestPath(1, 2).empty());

    invalidCostGraph.addRoad(1, 2, std::numeric_limits<double>::infinity());

    assert(std::isinf(invalidCostGraph.shortestDistance(1, 2)));
    assert(invalidCostGraph.shortestPath(1, 2).empty());

    invalidCostGraph.addRoad(1, 2, std::nan(""));

    assert(std::isinf(invalidCostGraph.shortestDistance(1, 2)));
    assert(invalidCostGraph.shortestPath(1, 2).empty());
   
    Graph blockTestGraph;
    blockTestGraph.addLocation(1);
    blockTestGraph.addLocation(2);

    blockTestGraph.setRoadBlocked(99, 100, true);
    blockTestGraph.setRoadBlocked(1, 99, true);

    assert(!blockTestGraph.isRoadBlocked(1, 2));
    assert(blockTestGraph.shortestDistance(1, 2) ==
       std::numeric_limits<double>::infinity());

        // BFS reachability tests
    Graph bfsGraph;

    bfsGraph.addRoad(1, 2, 5.0);
    bfsGraph.addRoad(2, 3, 4.0);
    bfsGraph.addRoad(3, 4, 2.0);

    // Connected locations should be reachable
    assert(bfsGraph.isReachable(1, 4));

    // A location should be reachable from itself
    assert(bfsGraph.isReachable(2, 2));

    // Unknown location should be unreachable
    assert(!bfsGraph.isReachable(1, 99));

    // Blocking a road should affect reachability
    bfsGraph.setRoadBlocked(2, 3, true);

    assert(!bfsGraph.isReachable(1, 4));

    // Unblocking should restore reachability
    bfsGraph.setRoadBlocked(2, 3, false);

    assert(bfsGraph.isReachable(1, 4));

    // BFS should handle cycles without revisiting locations indefinitely
    Graph cycleGraph;

    cycleGraph.addRoad(1, 2, 1.0);
    cycleGraph.addRoad(2, 3, 1.0);
    cycleGraph.addRoad(3, 4, 1.0);
    cycleGraph.addRoad(4, 1, 1.0);

    assert(cycleGraph.isReachable(1, 3));
    assert(cycleGraph.isReachable(1, 4));
    assert(!cycleGraph.isReachable(1, 99));

        // Zero-cost roads should be valid
    Graph zeroCostGraph;

    zeroCostGraph.addRoad(1, 2, 0.0);
    zeroCostGraph.addRoad(2, 3, 5.0);

    assert(zeroCostGraph.shortestDistance(1, 2) == 0.0);
    assert(zeroCostGraph.shortestDistance(1, 3) == 5.0);

    std::vector<int> zeroCostPath = {1, 2, 3};
    assert(zeroCostGraph.shortestPath(1, 3) == zeroCostPath);

        // Equal-cost alternative routes
    Graph equalCostGraph;

    equalCostGraph.addRoad(1, 2, 2.0);
    equalCostGraph.addRoad(2, 4, 3.0);

    equalCostGraph.addRoad(1, 3, 1.0);
    equalCostGraph.addRoad(3, 4, 4.0);

    assert(equalCostGraph.shortestDistance(1, 4) == 5.0);

    std::vector<int> path1 = {1, 2, 4};
    std::vector<int> path2 = {1, 3, 4};

    std::vector<int> actualPath = equalCostGraph.shortestPath(1, 4);

    assert(actualPath == path1 || actualPath == path2);

        // Multiple roads between the same locations
    Graph duplicateRoadGraph;

    duplicateRoadGraph.addRoad(1, 2, 5.0);
    duplicateRoadGraph.addRoad(1, 2, 3.0);

    assert(duplicateRoadGraph.shortestDistance(1, 2) == 3.0);

    std::vector<int> duplicatePath = {1, 2};
    assert(duplicateRoadGraph.shortestPath(1, 2) == duplicatePath);

    // Blocking the road blocks both parallel edges
    duplicateRoadGraph.setRoadBlocked(1, 2, true);

    assert(duplicateRoadGraph.isRoadBlocked(1, 2));
    assert(duplicateRoadGraph.isRoadBlocked(2, 1));

    assert(std::isinf(duplicateRoadGraph.shortestDistance(1, 2)));
    assert(duplicateRoadGraph.shortestPath(1, 2).empty());

    // Self-loop behaviour
    Graph selfLoopGraph;

    selfLoopGraph.addRoad(1, 1, 5.0);
    selfLoopGraph.addRoad(1, 2, 3.0);

    assert(selfLoopGraph.shortestDistance(1, 1) == 0.0);
    assert(selfLoopGraph.shortestDistance(1, 2) == 3.0);

    std::vector<int> selfLoopPath = {1};
    assert(selfLoopGraph.shortestPath(1, 1) == selfLoopPath);

    assert(selfLoopGraph.isReachable(1, 1));
    assert(selfLoopGraph.isReachable(1, 2));



    std::cout << "All Graph tests passed!\n";
    return 0;
}