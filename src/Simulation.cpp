#include "Simulation.h"

#include <iostream>
#include <stdexcept>

using namespace std;

Simulation::Simulation(ReliefManager& manager) : relief(manager) {}

void Simulation::schedule(int step, const string& description,
                          function<void(ReliefManager&)> action) {
    if (step <= 0) {
        throw invalid_argument("Event step must be positive.");
    }
    if (!action) {
        throw invalid_argument("Event needs an action.");
    }
    events.push_back({step, description, action});
}

int Simulation::run(int maxSteps) {
    if (maxSteps <= 0) {
        throw invalid_argument("maxSteps must be positive.");
    }

    int steps = 0;
    while (steps < maxSteps) {
        const int stepNumber = steps + 1;

        for (const Event& e : events) {
            if (e.step == stepNumber) {
                cout << "\n>>> Event before step " << stepNumber << ": "
                     << e.description << "\n";
                e.action(relief);
            }
        }

        if (!relief.hasUnmetDemand()) {
            break;
        }

        cout << "\n=== Step " << stepNumber << " ===\n";
        bool didWork = relief.runStep();
        steps++;

        if (!didWork) {
            // Only stop if no later event could change the situation.
            bool laterEvent = false;
            for (const Event& e : events) {
                if (e.step > stepNumber) laterEvent = true;
            }
            if (!laterEvent) {
                break;
            }
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
