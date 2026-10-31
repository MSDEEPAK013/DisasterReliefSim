
#pragma once

#include <unordered_map>
#include <vector>

struct Road {
    int to;
    double cost;
    bool blocked;
};

class Graph {
private:
    std::unordered_map<int, std::vector<Road>> adjacencyList;

    struct SearchResult {
    std::unordered_map<int, double> distance;
    std::unordered_map<int, int> previous;
};

    SearchResult runDijkstra(int start) const;

public:
    void addLocation(int id);

    void addRoad(int from, int to, double cost);

    void setRoadBlocked(int from, int to, bool blocked);

    bool isRoadBlocked(int from, int to) const;

    double shortestDistance(int start, int destination) const;
    std::vector<int> shortestPath(int start, int destination) const;
    bool isReachable(int start, int destination) const;
};
