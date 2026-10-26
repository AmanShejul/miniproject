# Smart Campus Navigation System — Submission Package

This folder contains a print-ready mini-project report, a complete source-code
listing, and genuine screenshots captured from the compiled terminal program.
The application is an offline, in-memory campus walking-path demonstrator; its
bundled sample locations and distances are fictional.

## Package contents

```text
submission/
  Project_Report.pdf
  Complete_Source_Code.pdf
  Output_Screenshots.pdf
  README_Submission.md
  source_code/
    main.c
    graph.c
    graph.h
    locations.c
    locations.h
    queue.c
    queue.h
    test_driver.c
    README_Project.md
    test_cli.ps1
  screenshots/
    01_main_menu.png
    02_sample_locations.png
    03_shortest_route.png
    04_bfs_traversal.png
    05_walking_paths.png
```

The test driver is a separate executable because it defines its own `main()`.
No executable or temporary build file is included in the source-code PDF.

## Build and run

From the project directory, compile the application (not `test_driver.c`) with
a C11-capable GCC:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -g main.c locations.c graph.c queue.c -o campus_navigation.exe
.\campus_navigation.exe
```

Compile and run the module test driver separately:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -g test_driver.c locations.c graph.c queue.c -o test_driver.exe
.\test_driver.exe
```

On the preparation machine, GCC 6.3.0 (MinGW) compiled both programs with the
requested warning flags and no compiler diagnostics. The module driver printed:

```text
Location, queue, and graph module tests passed.
```

The checked-in CLI regression script was also run against the fresh executable.
Its first two cases passed, then its third case failed because it expects
`That location ID is already in use.` while the application actually prints
`That location ID is already in the graph.`. No application or test source was
changed. A temporary copy of the test script with only that expected message
corrected passed all six CLI scenarios and all 13 EOF cases.

## Verified program behavior

- Adding and displaying locations; searching by ID or normalized,
  case-insensitive name.
- Connecting existing locations with positive undirected distances.
- Listing walking paths, finding minimum-distance routes with Dijkstra, BFS
  reachability/order, and graph connectivity.
- Rejecting malformed menu input, overlong names, duplicate locations, invalid
  distances, self-connections, and duplicate edges.
- Loading five explicitly fictional demonstration locations and seven paths.

This project does not implement live GPS, real-time traffic, a map service,
hash-based lookup, road closures, sorting, persistent storage, or editing and
deleting locations/paths through the menu.

## Personalization and printing

The report uses placeholders for student and institutional details. Fill these
in before submission. The certificate contains blank fields for authorized
signatures; no approval or signature has been supplied or fabricated.

All PDFs use A4 pages and include page numbers. Original captured PNG files are
in `screenshots/`. Captures document the menu, fictional sample locations and
paths, the 510 m shortest route, and BFS traversal. Location lookup and invalid
input are covered by command-line regression tests but are not included as
screenshots.
