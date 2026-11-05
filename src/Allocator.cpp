#include "Allocator.h"

#include <algorithm>
#include <limits>
#include <unordered_set>

using namespace std;

namespace {

    // unordered_map has no stable order, so ids are sorted first. That is what
    // makes ties (equal cost, equal capacity) always resolve to the lowest id.
    template <typename Map>
    vector<int> sortedIds(const Map& map) {
        vector<int> ids;
        ids.reserve(map.size());
        for (const auto& entry : map) {
            ids.push_back(entry.first);
        }
        sort(ids.begin(), ids.end());
        return ids;
    }

}

AllocationPlan Allocator::plan(ZoneQueue zones,
                               const unordered_map<int, ResourceCenter>& centers,
                               const unordered_map<int, Vehicle>& vehicles,
                               const Graph& graph,
                               int resourceId) {
    AllocationPlan result;

    const vector<int> centerIds = sortedIds(centers);
    const vector<int> vehicleIds = sortedIds(vehicles);

    // Working copies. Orders booked earlier in this plan use up stock and
    // vehicles, but the real objects stay untouched until the caller applies
    // the plan.
    unordered_map<int, int> stockLeft;
    for (int id : centerIds) {
        stockLeft[id] = centers.at(id).availableStock(resourceId);
    }
    unordered_set<int> busyVehicles;

    while (!zones.empty()) {
        const RankedZone ranked = zones.pop();
        const DisasterZone& zone = ranked.zone;

        int demandLeft = zone.unmetDemand;
        string reason;

        while (demandLeft > 0) {
            // --- 1. cheapest center with supply that can reach the zone ---
            int bestCenter = -1;
            double bestCost = numeric_limits<double>::infinity();
            bool anySupply = false;

            for (int id : centerIds) {
                if (stockLeft[id] <= 0) {
                    continue;
                }
                anySupply = true;

                if (!graph.isReachable(id, zone.id)) {
                    continue;   // no road path at all, skip the costlier Dijkstra
                }

                double cost = graph.shortestDistance(id, zone.id);
                // strictly smaller: ids are ascending, so on a tie the lower id stays
                if (cost < bestCost) {
                    bestCost = cost;
                    bestCenter = id;
                }
            }

            if (bestCenter < 0) {
                reason = anySupply ? "no center with supply can reach this zone"
                                   : "no supply left at any center";
                break;
            }

            // --- 2. biggest available vehicle that can get to that center ---
            // Roads are two-way, so a vehicle that can reach the center is in the
            // same connected area as the zone; no other center would do better.
            int bestVehicle = -1;
            int bestCapacity = 0;
            bool reachableButBusy = false; 

            for (int id : vehicleIds) {
                const Vehicle& vehicle = vehicles.at(id);
                if (!vehicle.isAvailable()) {                 
                    continue;
                }
                if (!graph.isReachable(vehicle.location(), bestCenter)) {
                    continue;
                }
                if (busyVehicles.count(id) > 0) {                       
                    reachableButBusy = true; 
                    continue;                                          
                } 
                if (vehicle.capacity() > bestCapacity) {
                    bestCapacity = vehicle.capacity();
                    bestVehicle = id;
                }
            }

            if (bestVehicle < 0) {
                reason = reachableButBusy
                    ? "every vehicle that can reach a center is already used this step"
                    : "no available vehicle can reach a center with supply";
                break;
            }

            // --- 3. book the delivery ---
            int quantity = min({demandLeft, stockLeft[bestCenter], bestCapacity});

            DispatchOrder order;
            order.zoneId = zone.id;
            order.centerId = bestCenter;
            order.vehicleId = bestVehicle;
            order.resourceId = resourceId;
            order.quantity = quantity;
            order.route.found = true;
            order.route.cost = bestCost;
            order.route.path = graph.shortestPath(bestCenter, zone.id);
            result.orders.push_back(order);

            stockLeft[bestCenter] -= quantity;
            busyVehicles.insert(bestVehicle);
            demandLeft -= quantity;
        }

        if (demandLeft > 0) {
            result.shortfalls.push_back({zone.id, demandLeft, reason});
        }
    }

    return result;
}