# Disaster Relief Resource Allocation and Routing System

First implementation increment: core disaster-zone data, resource inventory, and priority scoring.

## Build
```bash
cmake -S . -B build
cmake --build build
```

Run `./build/disaster_relief` (Windows executable location depends on the CMake generator).

## Current functionality
- Represents disaster zones with severity, affected population, and unmet demand.
- Scores zones using a weighted formula.
- Normalizes population and demand against scenario maxima.
- Sorts zones highest-to-lowest, using zone ID to break ties deterministically.
- Tracks resource stock and prevents removal beyond available inventory.

Starter weights are 0.50 for severity, 0.25 for affected population, and 0.25 for unmet demand. These are configurable implementation defaults, not validated disaster-response policy.

## Next increment
Add resource-demand-specific allocation logic and tests, then connect the model to routing.
