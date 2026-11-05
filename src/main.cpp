#include "DisasterZone.h"
#include "PriorityManager.h"
#include "ResourceCenter.h"
#include "ReliefManager.h"
#include "Vehicle.h"
#include "Resource.h"
#include "Simulation.h"
#include "Types.h"
#include "Console.h"
#include <fstream>
#include <exception>
#include <iomanip>
#include <iostream>
#include <vector>
using namespace std;

int main(int argc, char* argv[]) {
    cout << fixed << setprecision(3);
        if (argc >= 2) {
        ReliefManager relief;
        const string mode = argv[1];
        if (mode == "--interactive") {
            Console console(relief, cin, true);
            console.run();
            return 0;
        }
        if (mode == "--script" && argc >= 3) {
            ifstream file(argv[2]);
            if (!file) {
                cerr << "Cannot open " << argv[2] << "\n";
                return 1;
            }
            Console console(relief, file, false);
            console.run();
            return 0;
        }
        cerr << "Usage: disaster_relief [--interactive | --script <file>]\n";
        return 1;
    }
    
    // sample data for now
    const vector<DisasterZone> zones{
        {1, "North Sector", 5, 1200, 500},
        {2, "River District", 4, 2000, 300},
        {3, "East Village", 3, 800, 700}
    };

    // starter weights, we should decide these as a team
    const PriorityManager manager({0.50, 0.25, 0.25});

    try {
        vector<RankedZone> ranked = manager.rankZones(zones);

        cout << "Disaster Relief Resource Allocation and Routing System\n";
        cout << "Priority ranking (higher score = higher priority)\n\n";
        cout << fixed << setprecision(3);

        for (size_t i = 0; i < ranked.size(); i++) {
            cout << i + 1 << ". " << ranked[i].zone.name
                 << " | severity: " << ranked[i].zone.severity
                 << " | affected: " << ranked[i].zone.affectedPopulation
                 << " | unmet demand: " << ranked[i].zone.unmetDemand
                 << " | score: " << ranked[i].score << '\n';
        }

        // just checking the resource center works
        ResourceCenter warehouse(1, "Central Warehouse");
        warehouse.addStock(101, 100);
        cout << "\nWarehouse: " << warehouse.name()
             << " | resource 101 stock: " << warehouse.availableStock(101) << '\n';
    } catch (const exception& error) {
        cerr << "Error: " << error.what() << '\n';
        return 1;
    }


    Vehicle truck(1, 50, 0);
    Resource water(101, "Water", "litres");
    cout << "Vehicle " << truck.id() << " capacity " << truck.capacity()
         << " | Resource: " << water.name() << endl;

    cout << "\n--- Dispatch simulation ---\n";
    ReliefManager relief;
    relief.addZone({1, "North Sector", 5, 1200, 500});
    relief.addZone({2, "River District", 4, 2000, 300});
    relief.addZone({3, "East Village", 3, 800, 700});

    ResourceCenter depot(101, "Central Warehouse");
    depot.addStock(DEFAULT_SUPPLY_ID, 400);
    relief.addCenter(depot);
    relief.addVehicle(Vehicle(1, 100, 101)); 
    relief.setTravelSpeed(10.0);          // starts on a graph node

    relief.addLocation(3);                           // zone 3 exists but has no roads
    relief.addRoad(101, 201, 4);
    relief.addRoad(201, 1, 6);
    relief.addRoad(201, 2, 3);

    Simulation sim(relief);
    sim.schedule(2, "Road 201-1 is blocked", [](ReliefManager& r) {
        r.setRoadBlocked(201, 1, true);
    });
    sim.schedule(4, "Road 201-1 reopens", [](ReliefManager& r) {
        r.setRoadBlocked(201, 1, false);
    });
    sim.run(10);

    int unmet = 0;
    for (int id = 1; id <= 3; id++) unmet += relief.findZone(id)->unmetDemand;
    cout << "Stock left: " << relief.findCenter(101)->availableStock(DEFAULT_SUPPLY_ID)
         << " | total unmet demand: " << unmet << "\n";

    return 0;
}
