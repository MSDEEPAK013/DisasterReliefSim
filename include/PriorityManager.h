#pragma once
#include "DisasterZone.h"
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

class PriorityManager {
public:
    PriorityManager(PriorityWeights weights = PriorityWeights());

    double calculateScore(const DisasterZone& zone,
                          int maxAffectedPopulation,
                          int maxUnmetDemand) const;

    vector<RankedZone> rankZones(const vector<DisasterZone>& zones) const;

private:
    PriorityWeights weights;
};