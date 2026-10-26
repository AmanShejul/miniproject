# Smart Campus Navigation System in C

**Developer:** Swapnil Thite  
**Language:** C11  
**Interface:** Terminal / command line

## Objective

This college mini-project demonstrates how linked lists, an adjacency-matrix graph, a circular queue, Dijkstra's algorithm, and breadth-first search can support a small campus navigation system.

## Problem statement

A campus can be modeled as locations joined by walking paths. The application lets a user manage location records, describe paths with distances in metres, find a shortest-distance route, explore reachable locations, and check whether every location is connected to the rest of the map.

No actual campus map or measurements have been supplied. The optional sample data is fictional and exists only to demonstrate and test the program.

## Features

- Add and display locations stored in a singly linked list.
- Require positive, unique location IDs and names unique without regard to ASCII letter case.
- Search locations by ID or name.
- Connect two existing locations with a positive, undirected walking distance.
- Display all connected paths.
- Find and reconstruct a minimum-distance route with Dijkstra's algorithm.
- Traverse reachable locations with BFS using the custom circular queue.
- Check whether the graph is connected.
- Load five fictional demo locations and seven paths into an empty map.
- Validate terminal input with `fgets()` and `strtol()` and clean up allocated memory on exit.

## Data structures and algorithms

### Location linked list

Each node stores a positive ID, a name of up to 63 bytes, and a pointer to the next node. `LocationList` tracks its head, tail, and number of nodes. The list can grow dynamically; each new node is allocated with `malloc()` and released by `location_list_destroy()`.

The application checks the graph update and rolls it back if adding the corresponding list node fails. The list module itself has no graph dependency.

### Weighted graph

The graph has a fixed capacity of 100 vertices and uses a 100 × 100 adjacency matrix. A positive matrix value is the distance in metres; zero means there is no path. For each undirected path, the same distance is stored in both matrix directions.

The graph stores a location ID for every matrix row and column. Graph operations scan this ID array to find a vertex index. IDs such as 10, 73, and 420 therefore work without assuming that an ID determines its matrix position.

The matrix uses **O(V²)** space. Its fixed capacity keeps the representation easy to inspect and is suitable for this educational project.

### Dijkstra's shortest route

Dijkstra repeatedly selects the unvisited vertex with the smallest known distance, then relaxes its adjacent edges. A predecessor array records how each best distance was reached. The algorithm follows those predecessors backward from the destination and reverses the result to display the route in travel order.

The matrix implementation takes **O(V²)** time and **O(V)** additional space. It minimizes the sum of path distances. Distance addition is checked so a result outside the supported `int` range is reported instead of overflowing.

### BFS and the circular queue

BFS marks each vertex when it is enqueued, preventing it from entering the queue more than once. The queue is implemented in `queue.c` as a circular array with explicit empty and full checks. BFS returns reachable location IDs in traversal order.

With an adjacency matrix, BFS takes **O(V²)** time and **O(V)** additional space. BFS finds reachability and explores in increasing number of edges; it does not choose the least-distance route. Dijkstra minimizes total distance.

An empty graph is reported as not connected. A graph with one location is connected.

## Project files

```text
main.c          Menu, safe input, sample data, and module coordination
locations.h     Location and linked-list types, statuses, and declarations
locations.c     Linked-list operations and location searches
graph.h         Graph types, capacity, statuses, and declarations
graph.c         Adjacency matrix, Dijkstra, BFS, and connectivity
queue.h         Circular queue type and declarations
queue.c         Manual circular queue implementation
test_driver.c   Separate tests for the location, queue, and graph modules
README.md       Build, usage, algorithm, and test documentation
.gitignore      Generated executable and object-file patterns
```

## GCC setup

Install a C11-capable GCC toolchain and make sure `gcc` is available in the terminal. On Windows, the MSYS2 UCRT64 environment provides a GCC toolchain; its setup guide documents installation with `pacman`. On macOS, GCC is available through Homebrew. Linux distributions provide GCC through their package managers. See the [GNU GCC binary installation notes](https://gcc.gnu.org/install/binaries.html), [MSYS2 getting started guide](https://www.msys2.org/), and [Homebrew GCC formula](https://formulae.brew.sh/formula/gcc) for platform-specific details.

Verify the compiler before building:

```text
gcc --version
```

## Compile and run

Compile the application from the project directory:

```text
gcc -std=c11 -Wall -Wextra -Wpedantic -g main.c locations.c graph.c queue.c -o campus_navigation
```

Run it from PowerShell on Windows:

```powershell
.\campus_navigation.exe
```

Run it from a Linux or macOS terminal:

```sh
./campus_navigation
```

## Menu

1. Add campus location
2. Display all campus locations
3. Search for a location by ID or name
4. Connect two locations with a walking distance
5. Display connected paths
6. Find the shortest route using Dijkstra
7. Explore reachable locations using BFS and check a destination's reachability
8. Check graph connectivity
9. Load fictional sample data (only when the map is empty)
10. Exit

The map supports at most 100 locations. Names are trimmed at their ends, cannot be blank, and can contain at most 63 bytes (63 characters for ordinary ASCII names). Overlong terminal lines are discarded and reported. Numeric input must be a whole integer with no trailing text.

## Fictional sample execution

The sample IDs and distances below are invented for development; they do not describe a real campus.

```text
Choose an option: 9
Loaded five fictional demo locations and seven fictional paths.
These labels and distances are development examples, not a real campus map.

Choose an option: 6
Enter the starting location ID: 10
Enter the destination location ID: 900
Shortest route: Demo North Gate (ID 10) -> Demo Library (ID 73) -> Demo Cafeteria (ID 420) -> Demo Sports Centre (ID 900)
Total distance: 510 m
```

The route from ID 10 to ID 900 has an alternative through the Demo Science Block (260 + 80 + 220 = 560 m), so this example demonstrates why Dijkstra compares total distance rather than simply selecting any connected route.

## Tests

Build and run the separate module test driver:

```text
gcc -std=c11 -Wall -Wextra -Wpedantic -g test_driver.c locations.c graph.c queue.c -o test_driver
```

PowerShell:

```powershell
.\test_driver.exe
```

Linux or macOS:

```sh
./test_driver
```

The test driver checks:

- Successful insertion, duplicate IDs, case-insensitive duplicate names, invalid IDs, empty and overlong names, ID/name searches, and list cleanup/reset.
- Queue empty/full behavior, FIFO order, wraparound, and cleanup.
- Nonconsecutive graph IDs, undirected edges, invalid/self/duplicate paths, route reconstruction, alternative shortest routes, same-location routes, disconnected destinations, and output-buffer limits.
- BFS reachability/order, destination reachability, connected and disconnected graphs, vertex removal consistency inside the graph, the 100-vertex capacity, and distance-overflow reporting.

Expected successful driver output:

```text
Location, queue, and graph module tests passed.
```

The interactive menu should also be checked with malformed menu input, an out-of-range menu choice, an overlong name, and a duplicate-name insertion. If a duplicate name causes an add to fail, its proposed graph vertex must be rolled back; adding another location with that ID should then work.

## Limitations and future work

- The graph is limited to 100 vertices because it uses a fixed adjacency matrix.
- Locations and paths cannot currently be deleted or edited.
- Location and path data are held in memory only and are lost when the program exits.
- Names are compared case-insensitively for ASCII letters; this is not full Unicode case folding.
- Sample locations and distances are fictional and must be replaced with verified campus data before representing a real campus.

Possible future work includes synchronized location/path deletion, editing a path distance, and optional file-based persistence. These are outside the current required feature set.
