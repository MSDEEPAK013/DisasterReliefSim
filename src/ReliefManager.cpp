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

    cout << "[3] Reachability and [4] nearest-center route:\n";
    for (const RankedZone& rz : ranked) {
        const ResourceCenter* bestCenter = nullptr;
        Route bestRoute;
        for (const auto& entry : centers) {
            int centerNode = entry.second.id();   // center id doubles as node id
            int zoneNode = rz.zone.id;            // zone id doubles as node id
            if (!graph.isReachable(centerNode, zoneNode)) continue;   // BFS
            Route r = findRoute(centerNode, zoneNode);                // Dijkstra
            if (bestCenter == nullptr || r.cost < bestRoute.cost) {
                bestCenter = &entry.second;
                bestRoute = r;
            }
        }
        if (bestCenter == nullptr) {
            cout << "    " << rz.zone.name << ": UNREACHABLE\n";
        } else {
            cout << "    " << rz.zone.name << " <- " << bestCenter->name() << "\n";
            cout << "        Route: ";
            for (size_t i = 0; i < bestRoute.path.size(); i++) {
                if (i > 0) cout << " -> ";
                cout << nodeLabel(*this, bestRoute.path[i]);
            }
            cout << "\n        Cost: " << bestRoute.cost << "\n";
        }
    }

    // Stages 2-6 are placeholders until the other modules exist.
    cout << "[2] Allocate resources   - TODO (allocator)\n";
    cout << "[5] Dispatch vehicles    - TODO\n";
    cout << "[6] Update state         - TODO\n";
    return true;


}