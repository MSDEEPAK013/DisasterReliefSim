#pragma once

#include <stdexcept>
#include <vector>

struct Trip {
    int zoneId = -1;
    int centerId = -1;
    int quantity = 0;
    std::vector<int> path;     
    int deliverAt = 0;         
    int returnAt = 0;       
    int elapsed = 0;          
    bool delivered = false;
};

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
    bool isOnTrip() const { return onTrip; }        
    const Trip& trip() const { return currentTrip; }

    void setAvailable(bool value) { available = value; }
    void moveTo(int node) { currentNode = node; }

    void startTrip(const Trip& trip) {
        if (onTrip) {
            throw std::logic_error("Vehicle is already on a trip.");
        }
        currentTrip = trip;
        currentTrip.elapsed = 0;
        currentTrip.delivered = false;
        onTrip = true;
    }

    bool advance() {
        if (!onTrip) {
            return false;
        }
        currentTrip.elapsed++;
        bool justDelivered = false;
        if (!currentTrip.delivered && currentTrip.elapsed >= currentTrip.deliverAt) {
            currentTrip.delivered = true;
            justDelivered = true;
        }
        if (currentTrip.elapsed >= currentTrip.returnAt) {
            onTrip = false;
        }
        return justDelivered;
    }

private:
    int vehicleId;
    int maxCapacity;
    int currentNode;      // home node
    bool available = true;
    bool onTrip = false;
    Trip currentTrip;
};