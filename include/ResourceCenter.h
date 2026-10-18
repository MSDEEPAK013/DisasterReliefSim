#pragma once
#include <stdexcept>
#include <string>
#include <unordered_map>
using namespace std;

class ResourceCenter {
public:
    ResourceCenter(int id, string name) {
        centerId = id;
        centerName = name;
    }

    int id() const { return centerId; }
    const string& name() const { return centerName; }

    void addStock(int resourceId, int quantity) {
        if (quantity < 0) {
            throw invalid_argument("Stock cannot be negative.");
        }
        stock[resourceId] += quantity;
    }

    int availableStock(int resourceId) const {
        auto it = stock.find(resourceId);
        if (it == stock.end()) {
            return 0;
        }
        return it->second;
    }

    // returns false if there isn't enough stock to remove
    bool removeStock(int resourceId, int quantity) {
        if (quantity < 0) return false;

        auto it = stock.find(resourceId);
        if (it == stock.end() || it->second < quantity) {
            return false;
        }
        it->second -= quantity;
        return true;
    }

private:
    int centerId;
    string centerName;
    unordered_map<int, int> stock;   // resource id -> how much we have
};