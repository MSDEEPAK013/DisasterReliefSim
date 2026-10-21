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

    // Stages 2-6 are placeholders until the other modules exist.
    cout << "[2] Allocate resources   - TODO (allocator)\n";
    cout << "[3] Check reachability   - TODO (BFS/DFS)\n";
    cout << "[4] Find route           - TODO (Dijkstra)\n";
    cout << "[5] Dispatch vehicles    - TODO\n";
    cout << "[6] Update state         - TODO\n";
    return true;


}