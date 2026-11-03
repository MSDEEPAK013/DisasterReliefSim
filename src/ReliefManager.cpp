#include "ReliefManager.h"
#include <iostream>
#include <stdexcept>

using namespace std;

ReliefManager::ReliefManager(PriorityWeights weights) : priority(weights) {}

void ReliefManager::addZone(const DisasterZone& zone) {
    if (!zone.isValid()) {
        throw invalid_argument("Cannot add an invalid zone.");
    }
    if (zones.count(zone.id) > 0) {
        throw invalid_argument("A zone with this id already exists.");
    }
    zones.emplace(zone.id, zone);
}

void ReliefManager::addCenter(const ResourceCenter& center) {
    if (centers.count(center.id()) > 0) {
        throw invalid_argument("A center with this id already exists.");
    }
    centers.emplace(center.id(), center);
}

void ReliefManager::addVehicle(const Vehicle& vehicle) {
    if (vehicles.count(vehicle.id()) > 0) {
        throw invalid_argument("A vehicle with this id already exists.");
    }
    vehicles.emplace(vehicle.id(), vehicle);
}

void ReliefManager::addResource(const Resource& resource) {
    if (resources.count(resource.id()) > 0) {
        throw invalid_argument("A resource with this id already exists.");
    }
    resources.emplace(resource.id(), resource);
}

const DisasterZone* ReliefManager::findZone(int id) const {
    auto it = zones.find(id);
    return it == zones.end() ? nullptr : &it->second;
}

const ResourceCenter* ReliefManager::findCenter(int id) const {
    auto it = centers.find(id);
    return it == centers.end() ? nullptr : &it->second;
}

const Vehicle* ReliefManager::findVehicle(int id) const {
    auto it = vehicles.find(id);
    return it == vehicles.end() ? nullptr : &it->second;
}

const Resource* ReliefManager::findResource(int id) const {
    auto it = resources.find(id);
    return it == resources.end() ? nullptr : &it->second;
}

bool ReliefManager::hasUnmetDemand() const {
    for (const auto& entry : zones) {
        if (entry.second.unmetDemand > 0) return true;
    }
    return false;
}


void ReliefManager::addRoad(int from, int to, double cost) {
    graph.addRoad(from, to, cost);
}

void ReliefManager::setRoadBlocked(int from, int to, bool blocked) {
    graph.setRoadBlocked(from, to, blocked);
}

double ReliefManager::shortestDistance(int from, int to) const {
    return graph.shortestDistance(from, to);
}

std::vector<int> ReliefManager::shortestPath(int from, int to) const {
    return graph.shortestPath(from, to);
}

void ReliefManager::addLocation(int id) {
    graph.addLocation(id);
}

bool ReliefManager::isReachable(int from, int to) const {
    return graph.isReachable(from, to);
}

Route ReliefManager::findRoute(int from, int to) const {
    Route route;
    route.path = graph.shortestPath(from, to);
    route.found = !route.path.empty();
    if (route.found) {
        route.cost = graph.shortestDistance(from, to);
    }
    return route;
}
static string nodeLabel(const ReliefManager& m, int node) {
    if (const DisasterZone* z = m.findZone(node)) {
        return z->name + " (" + to_string(node) + ")";
    }
    if (const ResourceCenter* c = m.findCenter(node)) {
        return c->name() + " (" + to_string(node) + ")";
    }
    return "Junction " + to_string(node);
}

void ReliefManager::addStock(int centerId, int resourceId, int quantity) {
    auto it = centers.find(centerId);
    if (it == centers.end()) {
        throw invalid_argument("Unknown center id.");
    }
    it->second.addStock(resourceId, quantity);
}

void ReliefManager::setVehicleAvailable(int vehicleId, bool available) {
    auto it = vehicles.find(vehicleId);
    if (it == vehicles.end()) {
        throw invalid_argument("Unknown vehicle id.");
    }
    it->second.setAvailable(available);
}


bool ReliefManager::runStep(){

     // Only zones that still need help take part in this step.
    vector<DisasterZone> active;
    for (const auto& entry : zones) {
        if (entry.second.unmetDemand > 0) {
            active.push_back(entry.second);
        }
    }

    if (active.empty()) {
        cout << "No unmet demand. Nothing to do.\n";
        return false;
    }


    // Stage 1: rank zones (Sumukh's PriorityManager)
    vector<RankedZone> ranked = priority.rankZones(active);
    cout << "[1] Priority ranking:\n";
    for (size_t i = 0; i < ranked.size(); i++) {
        cout << "    " << (i + 1) << ". " << ranked[i].zone.name<< " (score " << ranked[i].score << ")\n";
    }

    cout << "[2] Reachability and Nearest-center route:\n";

    for (const RankedZone& rz : ranked) {
        const ResourceCenter* bestCenter = nullptr;
        Route bestRoute;

        for (const auto& entry : centers) {
            const ResourceCenter& center = entry.second;

            int centerNode = center.id();
            int zoneNode = rz.zone.id;

            if (!graph.isReachable(centerNode, zoneNode)) {
                continue;
            }

            Route route = findRoute(centerNode, zoneNode);

            if (!route.found) {
                continue;
            }

            if (bestCenter == nullptr || route.cost < bestRoute.cost) {
                bestCenter = &center;
                bestRoute = route;
            }
        }

        if (bestCenter == nullptr) {
            cout << "    " << rz.zone.name << ": UNREACHABLE\n";
        } else {
            cout << "    " << rz.zone.name
                 << " <- " << bestCenter->name() << "\n";

            cout << "        Route: ";

            for (size_t i = 0; i < bestRoute.path.size(); i++) {
                if (i > 0) cout << " -> ";
                cout << nodeLabel(*this, bestRoute.path[i]);
            }

            cout << "\n        Cost: " << bestRoute.cost << "\n";
        }
    }

    cout << "[3] Allocation Plan\n";
    AllocationPlan plan = Allocator::plan(priority.buildQueue(active),
                                          centers, vehicles, graph,
                                          DEFAULT_SUPPLY_ID);
    for (const DispatchOrder& o : plan.orders) {
        cout << "    " << findCenter(o.centerId)->name() << " -> "
             << findZone(o.zoneId)->name << ": " << o.quantity
             << " units by vehicle " << o.vehicleId
             << " (route cost " << o.route.cost << ")\n";
    }
    for (const Shortfall& s : plan.shortfalls) {
        cout << "    Shortfall: " << findZone(s.zoneId)->name << " still needs "
             << s.unserved << " units (" << s.reason << ")\n";
    }



    cout << "[4] Dispatch and state update:\n";
    for (const DispatchOrder& o : plan.orders) {
    centers.at(o.centerId).removeStock(o.resourceId, o.quantity);
    zones.at(o.zoneId).unmetDemand -= o.quantity;
    cout << "    Vehicle " << o.vehicleId << " delivered " << o.quantity
         << " units to " << findZone(o.zoneId)->name << "\n";
    }
    return !plan.orders.empty();   // false = nothing dispatched, so the loop stops

}