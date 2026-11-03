#include "Console.h"

#include "DisasterZone.h"
#include "ResourceCenter.h"
#include "Types.h"
#include "Vehicle.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace std;

namespace {
    // Names are typed with underscores: North_Sector -> "North Sector".
    string spaced(string text) {
        replace(text.begin(), text.end(), '_', ' ');
        return text;
    }
}

Console::Console(ReliefManager& manager, istream& input, bool interactive)
    : relief(manager), in(input), showPrompt(interactive) {}

void Console::run() {
    cout << "Disaster relief console. Type 'help' for commands.\n";
    string line;
    while (true) {
        if (showPrompt) {
            cout << "> " << flush;
        }
        if (!getline(in, line)) {
            break;
        }
        if (!showPrompt && !line.empty()) {
            cout << "> " << line << "\n";    // echo script lines so the output reads clearly
        }

        istringstream words(line);
        string command;
        if (!(words >> command) || command[0] == '#') {
            continue;
        }
        if (command == "quit" || command == "exit") {
            break;
        }

        try {
            if (!execute(command, words)) {
                cout << "Unknown command '" << command << "'. Type 'help'.\n";
            }
        } catch (const exception& error) {
            cout << "Error: " << error.what() << "\n";
        }
    }
}

bool Console::execute(const string& command, istringstream& args) {
    if (command == "help") {
        printHelp();
        return true;
    }

    if (command == "zone") {
        int id, severity, population, demand;
        string name;
        if (!(args >> id >> name >> severity >> population >> demand)) {
            cout << "Usage: zone <id> <name> <severity 1-5> <population> <demand>\n";
            return true;
        }
        if (relief.findCenter(id) != nullptr) {
            throw invalid_argument("That id belongs to a center. Zones and centers share map node numbers.");
        }
        DisasterZone zone;
        zone.id = id;
        zone.name = spaced(name);
        zone.severity = severity;
        zone.affectedPopulation = population;
        zone.unmetDemand = demand;
        relief.addZone(zone);
        relief.addLocation(id);          // zone id doubles as its node on the map
        zoneIds.push_back(id);
        cout << "Added zone " << id << " (" << zone.name << ").\n";
        return true;
    }

    if (command == "center") {
        int id, stock;
        string name;
        if (!(args >> id >> name >> stock)) {
            cout << "Usage: center <id> <name> <stock>\n";
            return true;
        }
        if (relief.findZone(id) != nullptr) {
            throw invalid_argument("That id belongs to a zone. Zones and centers share map node numbers.");
        }
        ResourceCenter center(id, spaced(name));
        if (stock > 0) {
            center.addStock(DEFAULT_SUPPLY_ID, stock);
        }
        relief.addCenter(center);
        relief.addLocation(id);          // center id doubles as its node on the map
        centerIds.push_back(id);
        cout << "Added center " << id << " (" << spaced(name) << ") with " << stock << " units.\n";
        return true;
    }

    if (command == "restock") {
        int id, amount;
        if (!(args >> id >> amount)) {
            cout << "Usage: restock <centerId> <amount>\n";
            return true;
        }
        relief.addStock(id, DEFAULT_SUPPLY_ID, amount);
        cout << "Added " << amount << " units to center " << id << ".\n";
        return true;
    }

    if (command == "vehicle") {
        int id, capacity, node;
        if (!(args >> id >> capacity >> node)) {
            cout << "Usage: vehicle <id> <capacity> <startNode>\n";
            return true;
        }
        relief.addVehicle(Vehicle(id, capacity, node));
        vehicleIds.push_back(id);
        cout << "Added vehicle " << id << " (capacity " << capacity << ", at node " << node << ").\n";
        return true;
    }

    if (command == "offline" || command == "online") {
        int id;
        if (!(args >> id)) {
            cout << "Usage: " << command << " <vehicleId>\n";
            return true;
        }
        relief.setVehicleAvailable(id, command == "online");
        cout << "Vehicle " << id << " is now " << command << ".\n";
        return true;
    }

    if (command == "node") {
        int id;
        if (!(args >> id)) {
            cout << "Usage: node <id>\n";
            return true;
        }
        relief.addLocation(id);
        cout << "Added node " << id << ".\n";
        return true;
    }

    if (command == "road") {
        int a, b;
        double cost;
        if (!(args >> a >> b >> cost)) {
            cout << "Usage: road <nodeA> <nodeB> <cost>\n";
            return true;
        }
        if (cost < 0) {
            throw invalid_argument("Road cost cannot be negative.");
        }
        if (a == b) {
            throw invalid_argument("A road needs two different nodes.");
        }
        relief.addRoad(a, b, cost);
        cout << "Added road " << a << " <-> " << b << " (cost " << cost << ").\n";
        return true;
    }

    if (command == "block" || command == "unblock") {
        int a, b;
        if (!(args >> a >> b)) {
            cout << "Usage: " << command << " <nodeA> <nodeB>\n";
            return true;
        }
        relief.setRoadBlocked(a, b, command == "block");
        cout << "Road " << a << " <-> " << b << " is now "
             << (command == "block" ? "blocked" : "open") << ".\n";
        return true;
    }

    if (command == "step" || command == "run") {
        int count = (command == "step") ? 1 : 100;
        string token;
        if (args >> token) {
            try {
                count = stoi(token);
            } catch (const exception&) {
                throw invalid_argument("Expected a number of steps.");
            }
        }
        if (count <= 0) {
            throw invalid_argument("Step count must be positive.");
        }
        advance(count);
        return true;
    }

    if (command == "status") {
        printStatus();
        return true;
    }

    if (command == "demo") {
        loadDemo();
        return true;
    }

    return false;
}

void Console::advance(int steps) {
    for (int i = 0; i < steps; i++) {
        if (!relief.hasUnmetDemand()) {
            cout << "All demand has been met.\n";
            return;
        }
        stepCount++;
        cout << "\n=== Step " << stepCount << " ===\n";
        if (!relief.runStep()) {
            cout << "Nothing could be dispatched this step. Change the map or stock, or type 'status'.\n";
            return;
        }
    }
}

void Console::printStatus() const {
    vector<int> ids;

    cout << "Zones:\n";
    ids = zoneIds;
    sort(ids.begin(), ids.end());
    for (int id : ids) {
        const DisasterZone* z = relief.findZone(id);
        cout << "  " << z->id << " " << z->name << " | severity " << z->severity
             << " | affected " << z->affectedPopulation
             << " | unmet demand " << z->unmetDemand << "\n";
    }
    if (ids.empty()) cout << "  (none)\n";

    cout << "Centers:\n";
    ids = centerIds;
    sort(ids.begin(), ids.end());
    for (int id : ids) {
        const ResourceCenter* c = relief.findCenter(id);
        cout << "  " << c->id() << " " << c->name()
             << " | stock " << c->availableStock(DEFAULT_SUPPLY_ID) << "\n";
    }
    if (ids.empty()) cout << "  (none)\n";

    cout << "Vehicles:\n";
    ids = vehicleIds;
    sort(ids.begin(), ids.end());
    for (int id : ids) {
        const Vehicle* v = relief.findVehicle(id);
        cout << "  " << v->id() << " | capacity " << v->capacity()
             << " | at node " << v->location()
             << " | " << (v->isAvailable() ? "online" : "offline") << "\n";
    }
    if (ids.empty()) cout << "  (none)\n";
}

void Console::printHelp() const {
    cout << "Commands:\n"
         << "  zone <id> <name> <severity 1-5> <population> <demand>\n"
         << "  center <id> <name> <stock>\n"
         << "  restock <centerId> <amount>\n"
         << "  vehicle <id> <capacity> <startNode>\n"
         << "  offline <vehicleId>   |   online <vehicleId>\n"
         << "  node <id>\n"
         << "  road <nodeA> <nodeB> <cost>\n"
         << "  block <nodeA> <nodeB>   |   unblock <nodeA> <nodeB>\n"
         << "  step [n]   run one (or n) steps\n"
         << "  run [n]    run up to n steps (default 100), stops when nothing can be dispatched\n"
         << "  status     show zones, centers and vehicles\n"
         << "  demo       load the three-zone demo map\n"
         << "  quit\n"
         << "Names use underscores for spaces. Zone ids, center ids and junction ids\n"
         << "are all map node numbers, so they must not overlap. Suggested ranges:\n"
         << "zones 1-99, centers 101-199, junctions 201 and up. A vehicle's start node\n"
         << "should be a center or junction on the map.\n";
}

void Console::loadDemo() {
    const char* lines[] = {
        "zone 1 North_Sector 5 1200 500",
        "zone 2 River_District 4 2000 300",
        "zone 3 East_Village 3 800 700",
        "center 101 Central_Warehouse 400",
        "vehicle 1 100 101",
        "road 101 201 4",
        "road 201 1 6",
        "road 201 2 3",
    };
    for (const char* line : lines) {
        istringstream words(line);
        string command;
        words >> command;
        execute(command, words);
    }
}
