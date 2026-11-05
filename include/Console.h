#pragma once

#include "ReliefManager.h"

#include <istream>
#include <sstream>
#include <string>
#include <vector>

// Reads commands from a stream (keyboard or script file) and applies them
// to a ReliefManager.
class Console {
public:
    Console(ReliefManager& manager, std::istream& input, bool interactive);

    // Runs until "quit" or the end of the input.
    void run();

private:
    // Returns false if the command word is not recognised.
    bool execute(const std::string& command, std::istringstream& args);
    void advance(int steps);
    void printStatus() const;
    void printHelp() const;
    void loadDemo();

    ReliefManager& relief;
    std::istream& in;
    bool showPrompt;
    int stepCount = 0;
    std::vector<int> zoneIds;
    std::vector<int> centerIds;
    std::vector<int> vehicleIds;
};