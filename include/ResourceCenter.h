#pragma once
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

class ResourceCenter {
public:
    ResourceCenter(int id, std::string name)
        : id_(id), name_(std::move(name)) {}

    int id() const { return id_; }
    const std::string& name() const { return name_; }

    void addStock(int resourceId, int quantity) {
        if (quantity < 0) throw std::invalid_argument("Stock cannot be negative.");
        stock_[resourceId] += quantity;
    }

    int availableStock(int resourceId) const {
        const auto it = stock_.find(resourceId);
        return it == stock_.end() ? 0 : it->second;
    }

    bool removeStock(int resourceId, int quantity) {
        if (quantity < 0) return false;
        auto it = stock_.find(resourceId);
        if (it == stock_.end() || it->second < quantity) return false;
        it->second -= quantity;
        return true;
    }

private:
    int id_;
    std::string name_;
    std::unordered_map<int, int> stock_;
};
