#include "DisasterZone.h"
#include "PriorityManager.h"
#include "ResourceCenter.h"
#include "ReliefManager.h"
#include "Vehicle.h"
#include "Resource.h"
#include "Simulation.h"
#include <exception>
#include <iomanip>
#include <iostream>
#include <vector>
using namespace std;

int main() {
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

    return 0;
}