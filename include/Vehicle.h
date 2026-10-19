#pragma once

#include <stdexcept>

// A delivery vehicle with a fixed carrying capacity.
class Vehicle {
public:
    Vehicle(int id, int capacity, int startNode) {
        if (id < 0) {
            throw std::invalid_argument("Vehicle id cannot be negative.");
        }
        if (capacity <= 0) {
            throw std::invalid_argument("Vehicle capacity must be positive.");
        }
        vehicleId = id;
        maxCapacity = capacity;
        currentNode = startNode;
    }

    int id() const { return vehicleId; }
    int capacity() const { return maxCapacity; }
    int location() const { return currentNode; }
    bool isAvailable() const { return available; }

    void setAvailable(bool value) { available = value; }
    void moveTo(int node) { currentNode = node; }

private:
    int vehicleId;
    int maxCapacity;
    int currentNode;      // graph node where the vehicle currently is
    bool available = true;
};