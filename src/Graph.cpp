#include "Graph.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <utility>
#include <unordered_set>

void Graph::addLocation(int id) {
    adjacencyList[id];
}

void Graph::addRoad(int from, int to, double cost) {
    if (cost < 0 || !std::isfinite(cost)) {
        return;
}
    adjacencyList[from].push_back({to, cost, false});
    adjacencyList[to].push_back({from, cost, false});
}


void Graph::setRoadBlocked(int from, int to, bool blocked) {
    auto fromIt = adjacencyList.find(from);
    auto toIt = adjacencyList.find(to);

    if (fromIt == adjacencyList.end() ||
        toIt == adjacencyList.end()) {
        return;
    }

    for (auto& road : fromIt->second) {
        if (road.to == to) {
            road.blocked = blocked;
        }
    }

    for (auto& road : toIt->second) {
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

Graph::SearchResult Graph::runDijkstra(int start) const {
    SearchResult result;

    if (adjacencyList.find(start) == adjacencyList.end()) {
        return result;
    }

    using Entry = std::pair<double, int>;

    std::priority_queue<
        Entry,
        std::vector<Entry>,
        std::greater<Entry>
    > pq;

    result.distance[start] = 0.0;
    pq.push({0.0, start});

    while (!pq.empty()) {
        auto [currentDistance, current] = pq.top();
        pq.pop();

        if (currentDistance != result.distance.at(current)) {
            continue;
        }

        for (const Road& road : adjacencyList.at(current)) {
            if (road.blocked) {
                continue;
            }

            double newDistance = currentDistance + road.cost;

            auto it = result.distance.find(road.to);

            if (it == result.distance.end() ||
                newDistance < it->second) {
                result.distance[road.to] = newDistance;
                result.previous[road.to] = current;
                pq.push({newDistance, road.to});
            }
        }
    }

    return result;
}

double Graph::shortestDistance(int start, int destination) const {
    const double INF = std::numeric_limits<double>::infinity();

    if (adjacencyList.find(start) == adjacencyList.end() ||
        adjacencyList.find(destination) == adjacencyList.end()) {
        return INF;
    }

    SearchResult result = runDijkstra(start);

    auto it = result.distance.find(destination);

    if (it == result.distance.end()) {
        return INF;
    }

    return it->second;
}

std::vector<int> Graph::shortestPath(
    int start, int destination
) const {
    if (adjacencyList.find(start) == adjacencyList.end() ||
        adjacencyList.find(destination) == adjacencyList.end()) {
        return {};
    }

    SearchResult result = runDijkstra(start);

    if (result.distance.find(destination) == result.distance.end()) {
        return {};
    }

    std::vector<int> path;
    int current = destination;

    while (current != start) {
        path.push_back(current);

        auto it = result.previous.find(current);

        if (it == result.previous.end()) {
            return {};
        }

        current = it->second;
    }

    

    path.push_back(start);
    std::reverse(path.begin(), path.end());

    return path;
}

bool Graph::isReachable(int start, int destination) const {

    if (adjacencyList.find(start) == adjacencyList.end() ||
        adjacencyList.find(destination) == adjacencyList.end()) {
        return false;
    }

    std::queue<int> q;
    std::unordered_set<int> visited;

    q.push(start);
    visited.insert(start);

    while (!q.empty()) {

        int current = q.front();
        q.pop();

        if (current == destination) {
            return true;
        }

        for (const Road& road : adjacencyList.at(current)) {

            if (road.blocked || visited.count(road.to)) {
                continue;
            }

            visited.insert(road.to);
            q.push(road.to);
        }
    }

    return false;
}