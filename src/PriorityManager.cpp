#include "PriorityManager.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
using namespace std;

bool PriorityWeights::areValid() const {
    if (severity < 0.0 || affectedPopulation < 0.0 || unmetDemand < 0.0) {
        return false;
    }
    // can't use == with doubles so check if its close enough to 1
    double sum = severity + affectedPopulation + unmetDemand;
    return fabs(sum - 1.0) < 1e-9;
}

PriorityManager::PriorityManager(PriorityWeights w) {
    if (!w.areValid()) {
        throw invalid_argument("Priority weights must be non-negative and sum to 1.");
    }
    weights = w;
}

double PriorityManager::calculateScore(const DisasterZone& zone,
                                       int maxAffectedPopulation,
                                       int maxUnmetDemand) const {
    if (!zone.isValid()) {
        throw invalid_argument("Invalid disaster zone.");
    }
    if (maxAffectedPopulation < 0 || maxUnmetDemand < 0) {
        throw invalid_argument("Normalization maxima cannot be negative.");
    }

    // scale everything to 0..1 so the weights actually mean something
    double sev = (double)zone.severity / 5.0;

    // max is 0 when every zone has 0, dividing would break so just use 0
    double pop = 0.0;
    if (maxAffectedPopulation != 0) {
        pop = (double)zone.affectedPopulation / maxAffectedPopulation;
    }

    double demand = 0.0;
    if (maxUnmetDemand != 0) {
        demand = (double)zone.unmetDemand / maxUnmetDemand;
    }

    return weights.severity * sev +
           weights.affectedPopulation * pop +
           weights.unmetDemand * demand;
}

// higher score first, smaller id first if the scores are the same
static bool higherPriority(const RankedZone& a, const RankedZone& b) {
    if (a.score != b.score) return a.score > b.score;
    return a.zone.id < b.zone.id;
}

bool ZoneQueue::LowerPriority::operator()(const RankedZone& a,
                                          const RankedZone& b) const {
    // a sits below b in the heap exactly when b has higher priority
    return higherPriority(b, a);
}

ZoneQueue::ZoneQueue(const vector<RankedZone>& scored)
    : heap(LowerPriority(), scored) {}   // O(n) build-heap

bool ZoneQueue::empty() const { return heap.empty(); }

size_t ZoneQueue::size() const { return heap.size(); }

void ZoneQueue::push(const RankedZone& zone) { heap.push(zone); }

const RankedZone& ZoneQueue::top() const {
    if (heap.empty()) {
        throw out_of_range("ZoneQueue is empty.");
    }
    return heap.top();
}

RankedZone ZoneQueue::pop() {
    if (heap.empty()) {
        throw out_of_range("ZoneQueue is empty.");
    }
    RankedZone best = heap.top();
    heap.pop();
    return best;
}

ZoneQueue PriorityManager::buildQueue(const vector<DisasterZone>& zones) const {
    int maxPopulation = 0;
    int maxDemand = 0;

    // find the maximums, also check the zones here
    for (int i = 0; i < (int)zones.size(); i++) {
        if (!zones[i].isValid()) {
            throw invalid_argument("Scenario contains an invalid zone.");
        }
        maxPopulation = max(maxPopulation, zones[i].affectedPopulation);
        maxDemand = max(maxDemand, zones[i].unmetDemand);
    }

    vector<RankedZone> scored;
    for (int i = 0; i < (int)zones.size(); i++) {
        RankedZone r;
        r.zone = zones[i];
        r.score = calculateScore(zones[i], maxPopulation, maxDemand);
        scored.push_back(r);
    }

    return ZoneQueue(scored);
}

vector<RankedZone> PriorityManager::rankZones(const vector<DisasterZone>& zones) const {
    ZoneQueue queue = buildQueue(zones);

    // popping everything from a heap gives a full ranking: O(n log n)
    vector<RankedZone> ranked;
    ranked.reserve(queue.size());
    while (!queue.empty()) {
        ranked.push_back(queue.pop());
    }
    return ranked;
}