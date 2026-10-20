#include "Graph.h"
#include <cassert>
#include <iostream>

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

    std::cout << "All Graph tests passed!\n";
    return 0;
}