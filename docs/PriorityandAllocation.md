# Priority and allocation: design notes

Covers the priority scoring, the heap-backed `ZoneQueue` and the greedy `Allocator`. It assumes the allocator pull requests are merged.

## Where it fits in a simulation step

```
active zones -> [1] PriorityManager ranks them (ZoneQueue, a heap)
             -> [2] Allocator plans deliveries   (this document)
             -> [3] reachability (BFS) and [4] route (Dijkstra), used by the allocator through Graph
             -> [5] dispatch and [6] state update (ReliefManager applies the plan)
```

The allocator only plans. It reads the centers, vehicles and road network and returns an `AllocationPlan`. Applying that plan (changing stock, demand and vehicle positions) is the manager's job, which keeps the allocator easy to test.

## Priority score

For a zone z, with the maxima taken over the zones being ranked:

```
Priority(z) = ws * (severity / 5) + wp * (population / maxPopulation) + wd * (unmetDemand / maxUnmetDemand)
```

Default weights are ws = 0.50, wp = 0.25, wd = 0.25 (they must be non-negative and sum to 1; the constructor throws otherwise). A zero maximum makes that term 0, so there is no division by zero. Ties are broken by the lower zone id, so the order is the same on every run.

Because the maxima come from the zones passed in, a zone's score can change from one step to the next when other zones are served. That is intended: the ranking reflects the zones still in need.

Worked example, weights 0.5/0.25/0.25, maxima 2000 people and 700 units:

| Zone | Severity | People | Demand | Score |
|---|---|---|---|---|
| North Sector | 5 | 1200 | 500 | 0.829 |
| River District | 4 | 2000 | 300 | 0.757 |
| East Village | 3 | 800 | 700 | 0.650 |

The weights are starting values chosen for the project, not validated disaster-response policy.

## ZoneQueue (heap)

`ZoneQueue` wraps `std::priority_queue`, a binary heap stored in a vector. The zone with the highest priority is always on top.

| Operation | Cost |
|---|---|
| build from n scored zones | O(n) |
| push | O(log n) |
| top | O(1) |
| pop | O(log n) |
| rank all n zones (pop everything) | O(n log n) |
| space | O(n) |

Draining the whole heap costs the same as sorting, so the heap does not make a full ranking faster. It pays off when the next zone is requested repeatedly and zones change in between, because nothing has to be re-sorted. `std::priority_queue` keeps its largest element on top, so the comparator answers "does a have lower priority than b?", which is why it calls `higherPriority(b, a)`.

## Allocator (greedy)

```
for each zone, most urgent first:
    while the zone still needs supply:
        center  = cheapest center that has stock and can reach the zone
                  (BFS: is there a road path?  Dijkstra: how costly is it?)
        vehicle = available vehicle with the biggest capacity that can reach that center,
                  not already used in this step
        book    min(demand left, stock left, vehicle capacity) units
        stop if no center or no vehicle fits, and record why
```

**Conventions and assumptions**

- A zone sits on the graph node with its own id, a center on the node with its id, a vehicle on its `location()`. A zone id and a center id must therefore never be equal.
- One kind of supply for now (`DEFAULT_SUPPLY_ID`, 101). `plan()` already takes a resource id, so several kinds later means one pass per id.
- A vehicle makes at most one trip per step. The cost of driving the vehicle to the center is not counted; only the center-to-zone route cost is.
- Equal costs and equal capacities are settled by the lower id, so results are reproducible.
- `plan()` never modifies its inputs.

**Complexity.** Let Z be zones, C centers, V vehicles, N locations and E roads. Taking zones off the heap is O(Z log Z). Each order scans the centers (a BFS plus a Dijkstra each) and the vehicles (a BFS each), so about O((C + V) (N + E) log N) per order. There are at most Z + V + C orders, because each order finishes a zone, uses up a vehicle for the step, or empties a center. The total is polynomial and fine for this project's sizes. Caching one Dijkstra run per center would cut it further.

**Why BFS before Dijkstra.** Dijkstra alone already returns an infinite distance for unreachable places, so the BFS check does not change any result. It is a cheap O(N + E) filter that skips the costlier shortest-path run, and it follows the BFS-then-Dijkstra order in the proposal.

**Limits of the greedy policy.** It is a heuristic, not an optimal allocation. A very large request from the most urgent zone can use up all the supply and leave several smaller zones with nothing, even though serving them first would have helped more people. A one-zone-at-a-time policy cannot see that.

## Calling the allocator from `ReliefManager::runStep`

Inside `runStep`, after ranking (this was checked in a scratch build, not committed):

```cpp
AllocationPlan plan = Allocator::plan(priority.buildQueue(active), centers, vehicles,
                                      graph, DEFAULT_SUPPLY_ID);

for (const DispatchOrder& order : plan.orders) {
    centers.at(order.centerId).removeStock(order.resourceId, order.quantity);
    zones.at(order.zoneId).unmetDemand -= order.quantity;
    vehicles.at(order.vehicleId).moveTo(order.zoneId);
}
```

A scratch run with 120 units of demand, two 50-unit vehicles and 500 units of stock delivered 100 units in step 1 and 20 in step 2, ended with no unmet demand and left 380 in stock. `runStep` can return `!plan.orders.empty()` so the simulation stops when nothing more can be delivered. `plan.shortfalls` lists each zone still short and the reason, ready to print or show in a GUI.

## Known issues to settle with the team

- **Shared ids.** `ScenarioBuilder` reuses small ids for both zones and centers (zones 1 to 3 and center 1 in most scenarios). With ids used as graph nodes, zone 1 and center 1 would be the same location.
- **No roads in the scenarios yet.** Nothing is reachable until `ScenarioBuilder` adds roads, and vehicles start at node 0, which is not in the graph.
- **Duplicate constant.** `ScenarioBuilder` has its own `SUPPLY_ID = 101`; it could use `DEFAULT_SUPPLY_ID` from `Types.h`.

## Tests

| File | Covers |
|---|---|
| `tests/PriorityManagerTests.cpp` | scoring, ranking, ties, bad weights, invalid zones, empty/single/tied/pushed heap, 200-zone ordering |
| `tests/AllocatorTests.cpp` | full delivery, route in the order, capacity limit, several vehicles, stock limit, priority order, one trip per step, inputs untouched, no vehicles, unavailable vehicle, unreachable vehicle or zone, blocked road, cheapest center, tie-break, second center taking over, equal priority, zero demand, other resource id, repeatability |
| `tests/ReliefManagerTests.cpp` | resource registry: add, find, duplicate rejected |

## Questions to be able to answer

- How does a binary heap keep the top element, and why is building one O(n)?
- Why is draining the heap O(n log n), and when is a heap better than sorting once?
- What does the greedy allocator do, and why is it not optimal? Give an example.
- Why check reachability with BFS when Dijkstra already reports unreachable places?
- How are ties made deterministic, and why does that matter for the tests?
- What would have to change to support several resource types?