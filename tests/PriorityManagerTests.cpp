
#include "PriorityManager.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int passed = 0;

void check(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error("FAILED: " + message);

    ++passed;
    std::cout << "PASS: " << message << '\n';
}

template <typename Function>
void checkThrows(Function function, const std::string& message) {
    try {
        function();
    } catch (const std::invalid_argument&) {
        check(true, message);
        return;
    }

    check(false, message);
}

int main() {
    try {
        PriorityManager manager({0.50, 0.25, 0.25});

        // 1. Empty input
        check(manager.rankZones({}).empty(),
              "Empty zone list returns empty ranking");

        // 2. Correct ranking
        const std::vector<DisasterZone> zones{
            {1, "North Sector", 5, 1200, 500},
            {2, "River District", 4, 2000, 300},
            {3, "East Village", 3, 800, 700}
        };

        const auto ranked = manager.rankZones(zones);

        check(ranked.size() == 3 &&
              ranked[0].zone.name == "North Sector" &&
              ranked[1].zone.name == "River District" &&
              ranked[2].zone.name == "East Village",
              "Zones are ranked by descending score");

        // 3. Invalid weights
        checkThrows([] {
            PriorityManager invalid({0.50, 0.25, 0.30});
        }, "Weights that do not sum to one are rejected");

        // 4. Invalid zone data
        checkThrows([&manager] {
            manager.rankZones({{1, "", 5, 100, 20}});
        }, "Invalid zone data is rejected");

        // 5. Zero population and demand
        const auto zeroResult = manager.rankZones({
            {1, "Zero Demand", 3, 0, 0}
        });

        check(std::abs(zeroResult[0].score - 0.30) < 1e-9,
              "Zero normalization maxima do not cause division by zero");

        // 6. Equal-score tie-breaking
        const auto tied = manager.rankZones({
            {2, "Zone B", 3, 100, 50},
            {1, "Zone A", 3, 100, 50}
        });

        check(tied[0].zone.id == 1 && tied[1].zone.id == 2,
              "Equal scores are resolved by ascending zone ID");

        std::cout << "\nAll " << passed
                  << " tests passed.\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "\nTEST FAILURE: "
                  << error.what() << '\n';
        return 1;
    }
}
