#include "PriorityManager.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <vector>
using namespace std;

int passed = 0;

void check(bool condition, string message) {
    if (!condition) {
        throw runtime_error("FAILED: " + message);
    }
    passed++;
    cout << "PASS: " << message << '\n';
}

// Returns true only if the action throws the given exception type.
template <typename Exception, typename Action>
bool throwsType(Action action) {
    try {
        action();
    } catch (const Exception&) {
        return true;
    }
    return false;
}

int main() {
    try {
        PriorityManager manager({0.50, 0.25, 0.25});

        // 1. empty input
        check(manager.rankZones({}).empty(),
              "Empty zone list returns empty ranking");

        // 2. normal ranking
        const vector<DisasterZone> zones{
            {1, "North Sector", 5, 1200, 500},
            {2, "River District", 4, 2000, 300},
            {3, "East Village", 3, 800, 700}
        };
        vector<RankedZone> ranked = manager.rankZones(zones);

        check(ranked.size() == 3 &&
              ranked[0].zone.name == "North Sector" &&
              ranked[1].zone.name == "River District" &&
              ranked[2].zone.name == "East Village",
              "Zones are ranked by descending score");

        // 3. bad weights (add up to 1.05)
        bool threw = false;
        try {
            PriorityManager invalid({0.50, 0.25, 0.30});
        } catch (const invalid_argument&) {
            threw = true;
        }
        check(threw, "Weights that do not sum to one are rejected");

        // 4. bad zone (empty name)
        threw = false;
        try {
            manager.rankZones({{1, "", 5, 100, 20}});
        } catch (const invalid_argument&) {
            threw = true;
        }
        check(threw, "Invalid zone data is rejected");

        // 5. zero population and demand, only severity counts: 0.5 * 3/5 = 0.30
        vector<RankedZone> zeroResult = manager.rankZones({
            {1, "Zero Demand", 3, 0, 0}
        });
        check(fabs(zeroResult[0].score - 0.30) < 1e-9,
              "Zero normalization maxima do not cause division by zero");

        // 6. same score, lower id should come first
        vector<RankedZone> tied = manager.rankZones({
            {2, "Zone B", 3, 100, 50},
            {1, "Zone A", 3, 100, 50}
        });
        check(tied[0].zone.id == 1 && tied[1].zone.id == 2,
              "Equal scores are resolved by ascending zone ID");


        // 7. heap queue: empty queue
        ZoneQueue emptyQueue = manager.buildQueue({});
        check(emptyQueue.empty() && emptyQueue.size() == 0,
              "Queue built from no zones is empty");
        check(throwsType<out_of_range>([&] { emptyQueue.top(); }),
              "top() on an empty queue throws out_of_range");
        check(throwsType<out_of_range>([&] { emptyQueue.pop(); }),
              "pop() on an empty queue throws out_of_range");

        // 8. heap queue: single zone
        ZoneQueue single = manager.buildQueue({{7, "Only Zone", 2, 50, 10}});
        check(single.size() == 1 && single.top().zone.id == 7,
              "Single-zone queue exposes that zone on top");
        check(single.pop().zone.id == 7 && single.empty(),
              "Popping the only zone empties the queue");

        // 9. heap queue: top() does not remove, pop() order follows priority
        ZoneQueue queue = manager.buildQueue(zones);
        check(queue.top().zone.name == "North Sector" && queue.size() == 3,
              "top() returns the best zone and does not remove it");
        check(queue.pop().zone.name == "North Sector" &&
              queue.pop().zone.name == "River District" &&
              queue.pop().zone.name == "East Village" &&
              queue.empty(),
              "pop() returns zones from highest to lowest priority");

        // 10. heap queue: ties come out by ascending id
        ZoneQueue tieQueue = manager.buildQueue({
            {3, "Zone C", 3, 100, 50},
            {1, "Zone A", 3, 100, 50},
            {2, "Zone B", 3, 100, 50}
        });
        check(tieQueue.pop().zone.id == 1 &&
              tieQueue.pop().zone.id == 2 &&
              tieQueue.pop().zone.id == 3,
              "Queue resolves equal scores by ascending zone ID");

        // 11. heap queue: push after build still keeps the order
        ZoneQueue pushQueue = manager.buildQueue(zones);
        RankedZone urgent;
        urgent.zone = {9, "Urgent", 5, 100, 100};
        urgent.score = 10.0;   // larger than any normalized score
        pushQueue.push(urgent);
        check(pushQueue.size() == 4 && pushQueue.top().zone.id == 9,
              "A pushed high-score zone moves to the top");

        // 12. heap queue: invalid zone is rejected
        check(throwsType<invalid_argument>([&] {
                  manager.buildQueue({{1, "", 5, 100, 20}});
              }),
              "buildQueue rejects an invalid zone");

        // 13. heap ranking matches an independent sort on a larger set
        vector<DisasterZone> many;
        unsigned state = 12345;   // tiny deterministic generator, no <random>
        auto next = [&state]() {
            state = state * 1103515245u + 12345u;
            return (state >> 16) & 0x7fff;
        };
        for (int id = 1; id <= 200; id++) {
            DisasterZone z;
            z.id = id;
            z.name = "Z" + to_string(id);
            z.severity = 1 + (int)(next() % 5);
            z.affectedPopulation = (int)(next() % 50) * 100;
            z.unmetDemand = (int)(next() % 20) * 10;
            many.push_back(z);
        }
        vector<RankedZone> viaHeap = manager.rankZones(many);

        vector<RankedZone> viaSort = viaHeap;   // same scores, re-sorted below
        sort(viaSort.begin(), viaSort.end(),
             [](const RankedZone& a, const RankedZone& b) {
                 if (a.score != b.score) return a.score > b.score;
                 return a.zone.id < b.zone.id;
             });
        bool sameOrder = viaHeap.size() == many.size();
        for (size_t i = 0; sameOrder && i < viaHeap.size(); i++) {
            sameOrder = viaHeap[i].zone.id == viaSort[i].zone.id;
        }
        check(sameOrder,
              "Heap ranking matches a plain sort on 200 zones");

        cout << "\nAll " << passed << " tests passed.\n";
        return 0;
    } catch (const exception& error) {
        cerr << "\nTEST FAILURE: " << error.what() << '\n';
        return 1;
    }
}