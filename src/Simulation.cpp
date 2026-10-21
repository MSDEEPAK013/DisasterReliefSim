#include "Simulation.h"

#include <iostream>
#include <stdexcept>

using namespace std;

Simulation::Simulation(ReliefManager& manager) : relief(manager) {}

int Simulation::run(int maxSteps) {
    if (maxSteps <= 0) {
        throw invalid_argument("maxSteps must be positive.");
    }

    int steps = 0;
    while (steps < maxSteps && relief.hasUnmetDemand()) {
        cout << "\n=== Step " << (steps + 1) << " ===\n";
        bool didWork = relief.runStep();
        steps++;
        if (!didWork) {
            break;   // nothing more can be done
        }
    }

    cout << "\n--- Simulation finished after " << steps << " step(s) ---\n";
    if (relief.hasUnmetDemand()) {
        if (steps >= maxSteps) {
            cout << "Stopped at the step limit; some demand is still unmet.\n";
        } else {
            cout << "Stopped early; some demand could not be served.\n";
        }
    } else {
        cout << "All demand has been met.\n";
    }
    return steps;
}