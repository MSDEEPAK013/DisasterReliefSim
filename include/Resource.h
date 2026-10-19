#pragma once

#include <stdexcept>
#include <string>

// A type of relief supply, such as food, water or medicine.
class Resource {
public:
    Resource(int id, std::string name, std::string unit = "units") {
        if (id < 0) {
            throw std::invalid_argument("Resource id cannot be negative.");
        }
        if (name.empty()) {
            throw std::invalid_argument("Resource name cannot be empty.");
        }
        resourceId = id;
        resourceName = name;
        resourceUnit = unit;
    }

    int id() const { return resourceId; }
    const std::string& name() const { return resourceName; }
    const std::string& unit() const { return resourceUnit; }

private:
    int resourceId;
    std::string resourceName;
    std::string resourceUnit;
};