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
    } catch (const exception& error) {
        cerr << "\nTEST FAILURE: " << error.what() << '\n';
        return 1;
    }
}