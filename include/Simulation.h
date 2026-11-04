#pragma once

#include "ReliefManager.h"

#include <functional>
#include <string>
#include <vector>

class Simulation {
public:
    explicit Simulation(ReliefManager& manager);

    // Registers an event that runs just before the given step (counting from 1).
    void schedule(int step, const std::string& description,
                  std::function<void(ReliefManager&)> action);

    int run(int maxSteps = 100);

private:
    struct Event {
        int step;
        std::string description;
        std::function<void(ReliefManager&)> action;
    };

    ReliefManager& relief;
    std::vector<Event> events;
};