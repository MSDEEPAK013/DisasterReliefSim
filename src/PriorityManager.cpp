#include "PriorityManager.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
using namespace std;

bool PriorityWeights::areValid() const {
    double epsilon = 1e-9;   // can't compare doubles with == so use a tiny tolerance
    if (severity < 0.0 || affectedPopulation < 0.0 || unmetDemand < 0.0) {
        return false;
    }
    return abs(severity + affectedPopulation + unmetDemand - 1.0) < epsilon;
}

PriorityManager::PriorityManager(PriorityWeights weights) : weights_(weights) {
    if (!weights_.areValid()) {
        throw invalid_argument("Priority weights must be non-negative and sum to 1.");
    }
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

    // scale each factor to 0..1 so the weights mean something
    double severity = (double)zone.severity / 5.0;

    // if the max is 0 then everyone has 0, so just use 0 instead of dividing
    double population = 0.0;
    if (maxAffectedPopulation != 0) {
        population = (double)zone.affectedPopulation / maxAffectedPopulation;
    }

    double demand = 0.0;
    if (maxUnmetDemand != 0) {
        demand = (double)zone.unmetDemand / maxUnmetDemand;
    }

    return weights_.severity * severity +
           weights_.affectedPopulation * population +
           weights_.unmetDemand * demand;
}

vector<RankedZone> PriorityManager::rankZones(const vector<DisasterZone>& zones) const {
    int maxPopulation = 0;
    int maxDemand = 0;

    // find the maxes (and check the zones while we're at it)
    for (const auto& zone : zones) {
        if (!zone.isValid()) {
            throw invalid_argument("Scenario contains an invalid zone.");
        }
        maxPopulation = max(maxPopulation, zone.affectedPopulation);
        maxDemand = max(maxDemand, zone.unmetDemand);
    }

    vector<RankedZone> ranked;
    for (const auto& zone : zones) {
        ranked.push_back({zone, calculateScore(zone, maxPopulation, maxDemand)});
    }

    // highest score first, if tied the smaller id goes first
    sort(ranked.begin(), ranked.end(), [](const RankedZone& a, const RankedZone& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.zone.id < b.zone.id;
    });

    return ranked;
}