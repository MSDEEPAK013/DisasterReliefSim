#include "PriorityManager.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
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

// for sort: higher score first, smaller id first if the scores are the same
bool higherPriority(const RankedZone& a, const RankedZone& b) {
    if (a.score != b.score) return a.score > b.score;
    return a.zone.id < b.zone.id;
}

vector<RankedZone> PriorityManager::rankZones(const vector<DisasterZone>& zones) const {
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

    vector<RankedZone> ranked;
    for (int i = 0; i < (int)zones.size(); i++) {
        RankedZone r;
        r.zone = zones[i];
        r.score = calculateScore(zones[i], maxPopulation, maxDemand);
        ranked.push_back(r);
    }

    // TODO: could use a heap instead of sorting everything, check later
    sort(ranked.begin(), ranked.end(), higherPriority);

    return ranked;
}