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
    std::cout << "All Graph tests passed!\n";
    return 0;
}