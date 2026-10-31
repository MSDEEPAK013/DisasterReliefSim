#pragma once
#include "DisasterZone.h"
#include <cstddef>
#include <queue>
#include <vector>
using namespace std;

struct PriorityWeights {
    double severity = 0.50;
    double affectedPopulation = 0.25;
    double unmetDemand = 0.25;

    bool areValid() const;
};

struct RankedZone {
    DisasterZone zone;
    double score = 0;
};

// Max-heap of ranked zones: the highest-priority zone is always on top.
// Higher score wins; on equal scores the smaller zone id wins (same rule
// as rankZones). Built on std::priority_queue, which is a binary heap.
class ZoneQueue {
public:
    ZoneQueue() = default;

    // Builds the heap from already-scored zones in O(n) (make_heap).
    explicit ZoneQueue(const vector<RankedZone>& scored);

    bool empty() const;
    size_t size() const;

    void push(const RankedZone& zone);          // O(log n)

    // Highest-priority zone without removing it. O(1).
    // Throws std::out_of_range if the queue is empty.
    const RankedZone& top() const;

    // Removes and returns the highest-priority zone. O(log n).
    // Throws std::out_of_range if the queue is empty.
    RankedZone pop();

private:
    // priority_queue keeps the "largest" element on top, so this returns
    // true when a has LOWER priority than b.
    struct LowerPriority {
        bool operator()(const RankedZone& a, const RankedZone& b) const;
    };

    priority_queue<RankedZone, vector<RankedZone>, LowerPriority> heap;
};

class PriorityManager {
public:
    PriorityManager(PriorityWeights weights = PriorityWeights());

    double calculateScore(const DisasterZone& zone,
                          int maxAffectedPopulation,
                          int maxUnmetDemand) const;

    // Validates and scores the zones, then returns them in a heap so the
    // caller can take the next most urgent zone one at a time.
    ZoneQueue buildQueue(const vector<DisasterZone>& zones) const;

    // All zones, highest priority first (drains the heap).
    vector<RankedZone> rankZones(const vector<DisasterZone>& zones) const;

private:
    PriorityWeights weights;
};