#pragma once

#include "ReliefManager.h"

// Drives a ReliefManager through repeated steps until all demand is met,
// no more progress is possible, or a step limit is reached.
class Simulation {
public:
    explicit Simulation(ReliefManager& manager);

    // Runs up to maxSteps steps. Returns the number of steps executed.
    int run(int maxSteps = 100);

private:
    ReliefManager& relief;   // not owned; the caller keeps it alive
};