
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

public:
    void addLocation(int id);

    void addRoad(int from, int to, double cost);

    void setRoadBlocked(int from, int to, bool blocked);

    bool isRoadBlocked(int from, int to) const;
};
