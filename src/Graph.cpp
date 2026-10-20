#include "Graph.h"

void Graph::addLocation(int id) {
    adjacencyList[id];
}

void Graph::addRoad(int from, int to, double cost) {
    adjacencyList[from].push_back({to, cost, false});
    adjacencyList[to].push_back({from, cost, false});
}

void Graph::setRoadBlocked(int from, int to, bool blocked) {
    for (auto& road : adjacencyList[from]) {
        if (road.to == to) {
            road.blocked = blocked;
        }
    }

    for (auto& road : adjacencyList[to]) {
        if (road.to == from) {
            road.blocked = blocked;
        }
    }
}

bool Graph::isRoadBlocked(int from, int to) const {
    auto it = adjacencyList.find(from);

    if (it == adjacencyList.end()) {
        return false;
    }

    for (const auto& road : it->second) {
        if (road.to == to) {
            return road.blocked;
        }
    }

    return false;
}
