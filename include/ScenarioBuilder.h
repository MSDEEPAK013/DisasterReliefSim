#pragma once

#include "ReliefManager.h"

// Fills a ReliefManager with ready-made test worlds.
// Resource id 101 is used as the standard supply in every scenario.
namespace ScenarioBuilder {

    // 3 zones, 1 well-stocked center, 2 vehicles. The normal case.
    void basic(ReliefManager& manager);

    // Total demand is far greater than the available stock.
    void insufficientSupply(ReliefManager& manager);

    // Zones and a center exist, but there are no vehicles.
    void noVehicles(ReliefManager& manager);

    // Two zones with identical scores, to test deterministic tie-breaking.
    void equalPriority(ReliefManager& manager);

    // Reproducible random world: the same seed always gives the same scenario.
    void random(ReliefManager& manager, unsigned int seed,
                int zoneCount, int centerCount, int vehicleCount);
}