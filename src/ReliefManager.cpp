#include "ReliefManager.h"
#include <algorithm>
#include <cmath>
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
    if (centers.count(zone.id) > 0) {                                                                  // ADD
        throw invalid_argument("This id belongs to a center. Zones and centers share map node ids.");  // ADD
    }
    zones.emplace(zone.id, zone);
}

void ReliefManager::addCenter(const ResourceCenter& center) {
    if (centers.count(center.id()) > 0) {
        throw invalid_argument("A center with this id already exists.");
    }
    if (zones.count(center.id()) > 0) {                                                                // ADD
        throw invalid_argument("This id belongs to a zone. Zones and centers share map node ids.");    // ADD
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

bool ReliefManager::hasLocation(int id) const {      // ADD (whole function)
    return graph.hasLocation(id);
}

void ReliefManager::setTravelSpeed(double distancePerStep) {
    if (!std::isfinite(distancePerStep) || distancePerStep < 0.0) {
        throw invalid_argument("Travel speed must be zero or positive.");
    }
    speed = distancePerStep;
}

bool ReliefManager::hasVehiclesOnTrip() const {
    for (const auto& entry : vehicles) {
        if (entry.second.isOnTrip()) return true;
    }
    return false;
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

// Whole steps needed to cover a distance at the current speed.
static int stepsFor(double cost, double speed) {
    if (speed <= 0.0 || !std::isfinite(cost)) return 0;
    return static_cast<int>(std::ceil(cost / speed - 1e-9));
}

bool ReliefManager::runStep(){

    // [0] Time passes: vehicles on the road move one step, arrivals hand over cargo.
    bool delivered = false;
    vector<int> travelling;
    for (const auto& entry : vehicles) {
        if (entry.second.isOnTrip()) travelling.push_back(entry.first);
    }
    sort(travelling.begin(), travelling.end());   // fixed order -> same output every run

    for (int id : travelling) {
        Vehicle& vehicle = vehicles.at(id);
        if (!vehicle.advance()) continue;

        const Trip& trip = vehicle.trip();
        DisasterZone& zone = zones.at(trip.zoneId);
        zone.unmetDemand = max(0, zone.unmetDemand - trip.quantity);
        committed[trip.zoneId] -= trip.quantity;

        if (!delivered) cout << "[0] Arrivals:\n";
        delivered = true;
        cout << "    Vehicle " << id << " delivered " << trip.quantity
             << " units to " << zone.name << "\n";
    }

    // Only zones with demand that is not already on its way take part in this step.
    vector<DisasterZone> active;
    for (const auto& entry : zones) {
        auto onTheRoad = committed.find(entry.first);
        int open = entry.second.unmetDemand - (onTheRoad == committed.end() ? 0 : onTheRoad->second);
        if (open > 0) {
            DisasterZone zone = entry.second;
            zone.unmetDemand = open;
            active.push_back(zone);
        }
    }

    if (active.empty()) {
        const bool inFlight = hasVehiclesOnTrip();
        cout << (inFlight ? "All remaining demand is already on its way.\n"
                          : "No unmet demand. Nothing to do.\n");
        return inFlight || delivered;
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

        bool anyStock = false;
        for (const auto& entry : centers) {
            const ResourceCenter& center = entry.second;

            if (center.availableStock(DEFAULT_SUPPLY_ID) <= 0) {
                continue;   // an empty center cannot serve anyone, so do not list it
            }
            anyStock = true;

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
            cout << "    " << rz.zone.name
                 << (anyStock ? ": UNREACHABLE\n" : ": NO STOCK at any center\n");
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
        DisasterZone& zone = zones.at(o.zoneId);

        if (speed <= 0.0) {
            // Instant mode: cargo arrives immediately, the vehicle stays home.
            zone.unmetDemand -= o.quantity;
            cout << "    Vehicle " << o.vehicleId << " delivered " << o.quantity
                 << " units to " << zone.name << "\n";
            continue;
        }

        Vehicle& vehicle = vehicles.at(o.vehicleId);
        const int home = vehicle.location();

        Trip trip;
        trip.zoneId = o.zoneId;
        trip.centerId = o.centerId;
        trip.quantity = o.quantity;

        // path: home -> center -> zone -> home (shared nodes are not repeated)
        trip.path = graph.shortestPath(home, o.centerId);
        trip.path.insert(trip.path.end(), o.route.path.begin() + 1, o.route.path.end());
        vector<int> back = graph.shortestPath(o.zoneId, home);
        if (back.size() > 1) {
            trip.path.insert(trip.path.end(), back.begin() + 1, back.end());
        }

        const double outCost = graph.shortestDistance(home, o.centerId) + o.route.cost;
        trip.deliverAt = max(1, stepsFor(outCost, speed));
        trip.returnAt = trip.deliverAt + stepsFor(graph.shortestDistance(o.zoneId, home), speed);

        vehicle.startTrip(trip);
        committed[o.zoneId] += o.quantity;

        cout << "    Vehicle " << o.vehicleId << " left with " << o.quantity
             << " units for " << zone.name << " (arrives in " << trip.deliverAt
             << " step(s), back home in " << trip.returnAt << ")\n";
    }

    // false = nothing is moving at all, so a Simulation may stop.
    return !plan.orders.empty() || hasVehiclesOnTrip() || delivered;
}