#include "PriorityManager.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

bool PriorityWeights::areValid() const {
    constexpr double epsilon = 1e-9;
    return severity >= 0.0 && affectedPopulation >= 0.0 && unmetDemand >= 0.0 &&
           std::abs(severity + affectedPopulation + unmetDemand - 1.0) < epsilon;
}

PriorityManager::PriorityManager(PriorityWeights weights) : weights_(weights) {
    if (!weights_.areValid()) {
        throw std::invalid_argument("Priority weights must be non-negative and sum to 1.");
    }
}

double PriorityManager::calculateScore(const DisasterZone& zone,
                                       int maxAffectedPopulation,
                                       int maxUnmetDemand) const {
    if (!zone.isValid()) throw std::invalid_argument("Invalid disaster zone.");
    if (maxAffectedPopulation < 0 || maxUnmetDemand < 0)
        throw std::invalid_argument("Normalization maxima cannot be negative.");

    const double severity = static_cast<double>(zone.severity) / 5.0;
    const double population = maxAffectedPopulation == 0 ? 0.0 :
        static_cast<double>(zone.affectedPopulation) / maxAffectedPopulation;
    const double demand = maxUnmetDemand == 0 ? 0.0 :
        static_cast<double>(zone.unmetDemand) / maxUnmetDemand;

    return weights_.severity * severity +
           weights_.affectedPopulation * population +
           weights_.unmetDemand * demand;
}

std::vector<RankedZone> PriorityManager::rankZones(
    const std::vector<DisasterZone>& zones) const {
    int maxPopulation = 0;
    int maxDemand = 0;

    for (const auto& zone : zones) {
        if (!zone.isValid()) throw std::invalid_argument("Scenario contains an invalid zone.");
        maxPopulation = std::max(maxPopulation, zone.affectedPopulation);
        maxDemand = std::max(maxDemand, zone.unmetDemand);
    }

    std::vector<RankedZone> ranked;
    ranked.reserve(zones.size());
    for (const auto& zone : zones) {
        ranked.push_back({zone, calculateScore(zone, maxPopulation, maxDemand)});
    }

    std::sort(ranked.begin(), ranked.end(), [](const RankedZone& a, const RankedZone& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.zone.id < b.zone.id;
    });
    return ranked;
}
