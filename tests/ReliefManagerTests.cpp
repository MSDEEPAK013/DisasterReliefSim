#include "ReliefManager.h"
#include "ScenarioBuilder.h"
#include "Simulation.h"
#include "Resource.h"
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

int passed = 0;

void check(bool condition, string message) {
    if (!condition) {
        throw runtime_error("FAILED: " + message);
    }
    passed++;
    cout << "PASS: " << message << '\n';
}

// Returns true only if the action throws std::invalid_argument.
template <typename Action>
bool throwsInvalid(Action action) {
    try {
        action();
    } catch (const invalid_argument&) {
        return true;
    }
    return false;
}

DisasterZone makeZone(int id, int demand) {
    DisasterZone z;
    z.id = id;
    z.name = "Zone " + to_string(id);
    z.severity = 3;
    z.affectedPopulation = 100;
    z.unmetDemand = demand;
    return z;
}

int main() {
    try {
        // 1. registration and lookup
        ReliefManager manager;
        manager.addZone(makeZone(1, 10));
        check(manager.findZone(1) != nullptr,
              "Added zone can be found by id");
        check(manager.findZone(99) == nullptr,
              "Unknown zone id returns nullptr");
        check(throwsInvalid([&] { manager.addZone(makeZone(1, 5)); }),
              "Duplicate zone id is rejected");

        DisasterZone badZone = makeZone(2, 5);
        badZone.severity = 9;   // valid range is 1 to 5
        check(throwsInvalid([&] { manager.addZone(badZone); }),
              "Invalid zone data is rejected");

        manager.addCenter(ResourceCenter(1, "Depot"));
        check(manager.findCenter(1) != nullptr,
              "Added center can be found by id");
        check(throwsInvalid([&] { manager.addCenter(ResourceCenter(1, "Again")); }),
              "Duplicate center id is rejected");

        manager.addVehicle(Vehicle(1, 50, 0));
        check(manager.findVehicle(1) != nullptr,
              "Added vehicle can be found by id");
        check(throwsInvalid([&] { manager.addVehicle(Vehicle(1, 20, 0)); }),
              "Duplicate vehicle id is rejected");

        // 2. unmet demand
        ReliefManager empty;
        check(!empty.hasUnmetDemand(),
              "Empty manager has no unmet demand");
        check(!empty.runStep(),
              "runStep on an empty manager does nothing");

        ReliefManager noDemand;
        noDemand.addZone(makeZone(1, 0));
        check(!noDemand.hasUnmetDemand(),
              "Zone with zero demand is not counted as unmet");
        check(manager.hasUnmetDemand(),
              "Zone with positive demand is counted as unmet");
        check(manager.runStep(),
              "runStep returns true when there is work to do");

        // 3. entity validation
        check(throwsInvalid([] { Vehicle v(1, 0, 0); }),
              "Vehicle with zero capacity is rejected");
        check(throwsInvalid([] { Vehicle v(-1, 10, 0); }),
              "Vehicle with negative id is rejected");
        check(throwsInvalid([] { Resource r(-1, "Water"); }),
              "Resource with negative id is rejected");
        check(throwsInvalid([] { Resource r(1, ""); }),
              "Resource with empty name is rejected");

        // 4. scenario reproducibility
        ReliefManager a, b, c;
        ScenarioBuilder::random(a, 42, 5, 2, 3);
        ScenarioBuilder::random(b, 42, 5, 2, 3);   // same seed as a
        ScenarioBuilder::random(c, 7, 5, 2, 3);    // different seed

        bool sameAsA = true;
        bool differsFromA = false;
        for (int i = 1; i <= 5; i++) {
            const DisasterZone* za = a.findZone(i);
            const DisasterZone* zb = b.findZone(i);
            const DisasterZone* zc = c.findZone(i);
            check(za != nullptr && zb != nullptr && zc != nullptr,
                  "Random scenario creates every requested zone");
            if (za->unmetDemand != zb->unmetDemand ||
                za->severity != zb->severity) {
                sameAsA = false;
            }
            if (za->unmetDemand != zc->unmetDemand) {
                differsFromA = true;
            }
        }
        check(sameAsA, "Same seed builds the same scenario");
        check(differsFromA, "Different seed builds a different scenario");

        // 5. simulation guard
        Simulation sim(empty);
        check(throwsInvalid([&] { sim.run(0); }),
              "Step limit of zero is rejected");
        check(sim.run(5) == 0,
              "Simulation with no demand runs zero steps");

        cout << "\nAll " << passed << " tests passed.\n";
        return 0;
    } catch (const exception& error) {
        cerr << "\nTEST FAILURE: " << error.what() << '\n';
        return 1;
    }
}