#pragma once
#include "DisasterZone.h"
#include <vector>

struct PriorityWeights {
    double severity{0.50};
    double affectedPopulation{0.25};
    double unmetDemand{0.25};
    bool areValid() const;
};

struct RankedZone {
    DisasterZone zone;
    double score{};
};

class PriorityManager {
public:
    explicit PriorityManager(PriorityWeights weights = {});
    double calculateScore(const DisasterZone& zone,
                          int maxAffectedPopulation,
                          int maxUnmetDemand) const;
    std::vector<RankedZone> rankZones(const std::vector<DisasterZone>& zones) const;

private:
    PriorityWeights weights_;
};
