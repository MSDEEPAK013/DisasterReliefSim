#include "DisasterZone.h"
#include "PriorityManager.h"
#include "ResourceCenter.h"
#include <exception>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    const std::vector<DisasterZone> zones{
        {1, "North Sector", 5, 1200, 500},
        {2, "River District", 4, 2000, 300},
        {3, "East Village", 3, 800, 700}
    };

    // Starter weights; the team should review these assumptions.
    const PriorityManager manager({0.50, 0.25, 0.25});

    try {
        const auto ranked = manager.rankZones(zones);
        std::cout << "Disaster Relief Resource Allocation and Routing System\n";
        std::cout << "Priority ranking (higher score = higher priority)\n\n";
        std::cout << std::fixed << std::setprecision(3);
        for (std::size_t i = 0; i < ranked.size(); ++i) {
            const auto& item = ranked[i];
            std::cout << i + 1 << ". " << item.zone.name
                      << " | severity: " << item.zone.severity
                      << " | affected: " << item.zone.affectedPopulation
                      << " | unmet demand: " << item.zone.unmetDemand
                      << " | score: " << item.score << '\n';
        }

        ResourceCenter warehouse(1, "Central Warehouse");
        warehouse.addStock(101, 100);
        std::cout << "\nWarehouse: " << warehouse.name()
                  << " | resource 101 stock: " << warehouse.availableStock(101) << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
