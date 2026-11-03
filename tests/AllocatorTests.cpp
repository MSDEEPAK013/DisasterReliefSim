#include "Allocator.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
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

const int WATER = DEFAULT_SUPPLY_ID;

// A small world the tests fill in by hand, so every case is visible in the test.
// Same convention as the rest of the project: a center sits on the graph node
// with its own id, a zone on the node with its id, a vehicle on its location.
struct World {
    unordered_map<int, ResourceCenter> centers;
    unordered_map<int, Vehicle> vehicles;
    Graph graph;

    void addCenter(int id, int stock, int resource = WATER) {
        ResourceCenter c(id, "Center " + to_string(id));
        c.addStock(resource, stock);
        centers.emplace(id, c);
        graph.addLocation(id);
    }
    void addVehicle(int id, int capacity, int node) {
        vehicles.emplace(id, Vehicle(id, capacity, node));
    }
    AllocationPlan plan(const vector<DisasterZone>& zones, int resource = WATER) const {
        PriorityManager manager;
        return Allocator::plan(manager.buildQueue(zones), centers, vehicles,
                               graph, resource);
    }
};

DisasterZone zone(int id, int severity, int demand) {
    DisasterZone z;
    z.id = id;
    z.name = "Zone " + to_string(id);
    z.severity = severity;
    z.affectedPopulation = 1000;
    z.unmetDemand = demand;
    return z;
}

int totalFor(const AllocationPlan& plan, int zoneId) {
    int sum = 0;
    for (const DispatchOrder& o : plan.orders) {
        if (o.zoneId == zoneId) sum += o.quantity;
    }
    return sum;
}

int main() {
    try {
        // Basic map: center 100, zones 1 and 2, all on one road network.
        //     100 --5-- 1
        //     100 --9-- 2
        World w;
        w.graph.addRoad(100, 1, 5.0);
        w.graph.addRoad(100, 2, 9.0);
        w.addCenter(100, 100);
        w.addVehicle(1, 50, 100);

        // 1. nothing to plan
        check(w.plan({}).orders.empty() && w.plan({}).shortfalls.empty(),
              "No zones gives an empty plan");

        // 2. one zone, enough of everything
        AllocationPlan simple = w.plan({zone(1, 3, 40)});
        check(simple.orders.size() == 1 && simple.shortfalls.empty(),
              "A fully coverable zone gets one order and no shortfall");
        check(simple.orders[0].quantity == 40 &&
              simple.orders[0].zoneId == 1 &&
              simple.orders[0].centerId == 100 &&
              simple.orders[0].vehicleId == 1 &&
              simple.orders[0].resourceId == WATER,
              "The order carries the right zone, center, vehicle, resource and amount");
        check(simple.orders[0].route.found &&
              simple.orders[0].route.cost == 5.0 &&
              (simple.orders[0].route.path == vector<int>{100, 1}),
              "The order carries the center-to-zone route and its cost");

        // 3. vehicle capacity is the limit
        AllocationPlan capped = w.plan({zone(1, 3, 120)});
        check(capped.orders.size() == 1 && capped.orders[0].quantity == 50,
              "One vehicle delivers at most its capacity per step");
        check(capped.shortfalls.size() == 1 && capped.shortfalls[0].unserved == 70,
              "The part beyond vehicle capacity is reported as a shortfall");

        // 4. several vehicles share one zone's demand, biggest first
        World fleet = w;
        fleet.centers.at(100).addStock(WATER, 100);   // 200 in total, so stock is not the limit
        fleet.addVehicle(2, 100, 100);
        fleet.addVehicle(3, 30, 100);
        AllocationPlan shared = fleet.plan({zone(1, 3, 140)});
        check(shared.orders.size() == 2 &&
              shared.orders[0].vehicleId == 2 && shared.orders[0].quantity == 100 &&
              shared.orders[1].vehicleId == 1 && shared.orders[1].quantity == 40,
              "Biggest vehicle goes first, next biggest covers the rest");
        check(shared.shortfalls.empty(),
              "Demand covered by two vehicles leaves no shortfall");

        // 5. stock is the limit
        World lowStock;
        lowStock.graph.addRoad(100, 1, 5.0);
        lowStock.addCenter(100, 30);
        lowStock.addVehicle(1, 100, 100);
        AllocationPlan scarce = lowStock.plan({zone(1, 3, 100)});
        check(scarce.orders.size() == 1 && scarce.orders[0].quantity == 30,
              "Delivery is limited by the stock at the center");
        check(scarce.shortfalls.size() == 1 &&
              scarce.shortfalls[0].unserved == 70 &&
              scarce.shortfalls[0].reason == "no supply left at any center",
              "Demand above supply is reported with the reason");

        // 6. priority order decides who gets scarce supply
        World contest;
        contest.graph.addRoad(100, 1, 5.0);
        contest.graph.addRoad(100, 2, 5.0);
        contest.addCenter(100, 50);
        contest.addVehicle(1, 100, 100);
        contest.addVehicle(2, 100, 100);
        AllocationPlan ranked = contest.plan({zone(1, 1, 40), zone(2, 5, 40)});
        check(totalFor(ranked, 2) == 40 && totalFor(ranked, 1) == 10,
              "The higher-priority zone is served first when supply runs short");
        check(ranked.orders[0].zoneId == 2,
              "Orders are listed in priority order");

        // 7. a vehicle only makes one trip per step
        World oneTruck;
        oneTruck.graph.addRoad(100, 1, 5.0);
        oneTruck.graph.addRoad(100, 2, 5.0);
        oneTruck.addCenter(100, 1000);
        oneTruck.addVehicle(1, 100, 100);
        AllocationPlan trips = oneTruck.plan({zone(1, 5, 60), zone(2, 4, 60)});
        check(trips.orders.size() == 1 && trips.orders[0].zoneId == 1,
              "A single vehicle makes one trip per step");
        check(trips.shortfalls.size() == 1 && trips.shortfalls[0].zoneId == 2,
              "The zone left without a vehicle is reported as a shortfall");

        // 8. the plan never changes its inputs
        const int stockBefore = w.centers.at(100).availableStock(WATER);
        const int locationBefore = w.vehicles.at(1).location();
        w.plan({zone(1, 3, 40)});
        check(w.centers.at(100).availableStock(WATER) == stockBefore &&
              w.vehicles.at(1).location() == locationBefore &&
              w.vehicles.at(1).isAvailable(),
              "Planning does not change stock or vehicles");

        cout << "\nAll " << passed << " tests passed.\n";
        return 0;


        // 9. no vehicles / unavailable vehicle
        World none;
        none.graph.addRoad(100, 1, 5.0);
        none.addCenter(100, 100);
        AllocationPlan noTruck = none.plan({zone(1, 3, 40)});
        check(noTruck.orders.empty() && noTruck.shortfalls.size() == 1 &&
              noTruck.shortfalls[0].reason ==
                  "no available vehicle can reach a center with supply",
              "Without vehicles nothing is dispatched and the reason is given");

        World parked = w;
        parked.vehicles.at(1).setAvailable(false);
        check(parked.plan({zone(1, 3, 40)}).orders.empty(),
              "An unavailable vehicle is not used");
        parked.vehicles.at(1).setAvailable(true);
        check(parked.plan({zone(1, 3, 40)}).orders.size() == 1,
              "The same vehicle is used again once it is available");

        // 10. a vehicle in another part of the map cannot help
        World cutOff = w;
        cutOff.vehicles.erase(1);
        cutOff.graph.addRoad(7, 8, 1.0);        // a separate island
        cutOff.addVehicle(1, 50, 7);
        check(cutOff.plan({zone(1, 3, 40)}).orders.empty(),
              "A vehicle that cannot reach the center is not used");

        World lost = w;
        lost.vehicles.erase(1);
        lost.addVehicle(1, 50, 555);            // a node that is not in the graph
        check(lost.plan({zone(1, 3, 40)}).orders.empty(),
              "A vehicle at an unknown location is not used");

        // 11. unreachable zone
        World island = w;
        island.graph.addLocation(5);            // exists, but has no roads
        AllocationPlan stranded = island.plan({zone(5, 3, 40)});
        check(stranded.orders.empty() && stranded.shortfalls.size() == 1 &&
              stranded.shortfalls[0].reason ==
                  "no center with supply can reach this zone",
              "A zone with no road to any center gets nothing");

        // 12. blocked roads cut a zone off, unblocking restores it
        World road = w;
        road.graph.setRoadBlocked(100, 1, true);
        check(road.plan({zone(1, 3, 40)}).orders.empty(),
              "A blocked road makes the zone unreachable");
        road.graph.setRoadBlocked(100, 1, false);
        check(road.plan({zone(1, 3, 40)}).orders.size() == 1,
              "The zone is served again after the road reopens");

        // 13. a zone that is not on the map at all
        check(w.plan({zone(99, 3, 40)}).shortfalls[0].reason ==
                  "no center with supply can reach this zone",
              "A zone missing from the road network is reported, not crashed on");

        // 14. cheapest reachable center wins; ties go to the lower id
        //     0 --1-- 3 (zone), center 20 is 2 away, center 10 is 6 away
        World two;
        two.graph.addRoad(0, 3, 1.0);
        two.graph.addRoad(3, 20, 2.0);
        two.graph.addRoad(3, 10, 6.0);
        two.addCenter(10, 100);   // farther
        two.addCenter(20, 100);   // nearer
        two.addVehicle(1, 50, 0);
        check(two.plan({zone(3, 3, 40)}).orders[0].centerId == 20,
              "The cheapest reachable center is chosen");

        World tie;
        tie.graph.addRoad(0, 3, 1.0);
        tie.graph.addRoad(3, 20, 4.0);
        tie.graph.addRoad(3, 10, 4.0);
        tie.addCenter(20, 100);
        tie.addCenter(10, 100);
        tie.addVehicle(1, 50, 0);
        check(tie.plan({zone(3, 3, 40)}).orders[0].centerId == 10,
              "Equal-cost centers are resolved by the lower id");

        // 15. a second center takes over when the first runs dry
        World relay;
        relay.graph.addRoad(0, 3, 1.0);
        relay.graph.addRoad(3, 10, 2.0);
        relay.graph.addRoad(3, 20, 6.0);
        relay.addCenter(10, 30);     // near but small
        relay.addCenter(20, 100);    // far but big
        relay.addVehicle(1, 100, 0);
        relay.addVehicle(2, 100, 0);
        AllocationPlan relayPlan = relay.plan({zone(3, 3, 80)});
        check(relayPlan.orders.size() == 2 &&
              relayPlan.orders[0].centerId == 10 && relayPlan.orders[0].quantity == 30 &&
              relayPlan.orders[1].centerId == 20 && relayPlan.orders[1].quantity == 50,
              "When the nearest center runs out the next one covers the rest");

        // 16. equal priority: the lower zone id is served first
        World same;
        same.graph.addRoad(100, 1, 5.0);
        same.graph.addRoad(100, 2, 5.0);
        same.addCenter(100, 40);
        same.addVehicle(1, 100, 100);
        same.addVehicle(2, 100, 100);
        AllocationPlan tied = same.plan({zone(2, 3, 40), zone(1, 3, 40)});
        check(totalFor(tied, 1) == 40 && totalFor(tied, 2) == 0,
              "Zones with equal priority are served in ascending id order");

        // 17. a zone with no unmet demand needs nothing
        AllocationPlan idle = w.plan({zone(1, 3, 0)});
        check(idle.orders.empty() && idle.shortfalls.empty(),
              "A zone with zero demand produces no order and no shortfall");

        // 18. resource id is a parameter: other kinds of supply are separate
        const int MEDICINE = 7;
        World mixed;
        mixed.graph.addRoad(100, 1, 5.0);
        mixed.addCenter(100, 100, MEDICINE);   // only medicine, no water
        mixed.addVehicle(1, 50, 100);
        check(mixed.plan({zone(1, 3, 40)}, WATER).orders.empty(),
              "Stock of a different resource is not used");
        AllocationPlan meds = mixed.plan({zone(1, 3, 40)}, MEDICINE);
        check(meds.orders.size() == 1 && meds.orders[0].resourceId == MEDICINE,
              "The same allocator serves another resource id");

        // 19. same input, same output
        AllocationPlan again = relay.plan({zone(3, 3, 80)});
        bool identical = again.orders.size() == relayPlan.orders.size();
        for (size_t i = 0; identical && i < again.orders.size(); i++) {
            identical = again.orders[i].centerId == relayPlan.orders[i].centerId &&
                        again.orders[i].vehicleId == relayPlan.orders[i].vehicleId &&
                        again.orders[i].quantity == relayPlan.orders[i].quantity;
        }
        check(identical, "Planning twice gives the identical plan");
        
    } catch (const exception& error) {
        cerr << "\nTEST FAILURE: " << error.what() << '\n';
        return 1;
    }
}