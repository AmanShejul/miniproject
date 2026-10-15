#include "graph.h"
#include "locations.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_LINE_CAPACITY 256

typedef enum {
    INPUT_RESULT_OK,
    INPUT_RESULT_INVALID,
    INPUT_RESULT_EOF
} InputResult;

static InputResult read_line(const char *prompt, char *buffer, size_t capacity)
{
    size_t length;

    if (buffer == NULL || capacity < 2) {
        return INPUT_RESULT_INVALID;
    }
    if (prompt != NULL) {
        fputs(prompt, stdout);
        fflush(stdout);
    }
    if (fgets(buffer, (int)capacity, stdin) == NULL) {
        return INPUT_RESULT_EOF;
    }

    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
        if (length > 0 && buffer[length - 1] == '\r') {
            buffer[--length] = '\0';
        }
    } else if (!feof(stdin)) {
        int character;
        while ((character = getchar()) != '\n' && character != EOF) {
            /* Discard the rest of this overlong line. */
        }
        puts("Input line is too long; please try again.");
        return INPUT_RESULT_INVALID;
    }

    return INPUT_RESULT_OK;
}

static InputResult read_integer(const char *prompt, int *value)
{
    char line[INPUT_LINE_CAPACITY];
    char *end;
    long parsed_value;
    InputResult result;

    if (value == NULL) {
        return INPUT_RESULT_INVALID;
    }

    result = read_line(prompt, line, sizeof(line));
    if (result != INPUT_RESULT_OK) {
        return result;
    }

    errno = 0;
    parsed_value = strtol(line, &end, 10);
    if (end == line || errno == ERANGE || parsed_value < INT_MIN ||
        parsed_value > INT_MAX) {
        puts("Invalid input; please enter a whole number.");
        return INPUT_RESULT_INVALID;
    }
    while (isspace((unsigned char)*end)) {
        end++;
    }
    if (*end != '\0') {
        puts("Invalid input; please enter a whole number without extra text.");
        return INPUT_RESULT_INVALID;
    }

    *value = (int)parsed_value;
    return INPUT_RESULT_OK;
}

static void print_location_reference(const LocationList *locations, int id)
{
    const Location *location = location_find_by_id(locations, id);

    if (location == NULL) {
        printf("ID %d", id);
    } else {
        printf("%s (ID %d)", location->name, location->id);
    }
}

static bool add_location_action(LocationList *locations, Graph *graph)
{
    int id;
    char name[INPUT_LINE_CAPACITY];
    InputResult input_result;
    GraphStatus graph_status;
    LocationStatus location_status;

    input_result = read_integer("Enter a positive location ID: ", &id);
    if (input_result == INPUT_RESULT_EOF) {
        return false;
    }
    if (input_result != INPUT_RESULT_OK) {
        return true;
    }

    input_result = read_line("Enter the location name (up to 63 characters): ",
                             name, sizeof(name));
    if (input_result == INPUT_RESULT_EOF) {
        return false;
    }
    if (input_result != INPUT_RESULT_OK) {
        return true;
    }

    graph_status = graph_add_vertex(graph, id);
    if (graph_status != GRAPH_STATUS_OK) {
        printf("Location was not added: %s\n",
               graph_status_message(graph_status));
        return true;
    }

    location_status = location_list_add(locations, id, name);
    if (location_status != LOCATION_STATUS_OK) {
        GraphStatus rollback_status = graph_remove_vertex(graph, id);
        printf("Location was not added: %s\n",
               location_status_message(location_status));
        if (rollback_status != GRAPH_STATUS_OK) {
            puts("Internal consistency error: graph rollback failed.");
        }
        return true;
    }

    printf("Added %s (ID %d).\n",
           location_find_by_id(locations, id)->name, id);
    return true;
}

static bool search_location_action(const LocationList *locations)
{
    int search_type;
    InputResult input_result;
    const Location *location = NULL;

    input_result = read_integer("Search by 1) ID or 2) name: ", &search_type);
    if (input_result == INPUT_RESULT_EOF) {
        return false;
    }
    if (input_result != INPUT_RESULT_OK) {
        return true;
    }

    if (search_type == 1) {
        int id;
        input_result = read_integer("Enter the location ID: ", &id);
        if (input_result == INPUT_RESULT_EOF) {
            return false;
        }
        if (input_result != INPUT_RESULT_OK) {
            return true;
        }
        location = location_find_by_id(locations, id);
    } else if (search_type == 2) {
        char name[INPUT_LINE_CAPACITY];
        input_result = read_line("Enter the location name: ", name,
                                 sizeof(name));
        if (input_result == INPUT_RESULT_EOF) {
            return false;
        }
        if (input_result != INPUT_RESULT_OK) {
            return true;
        }
        location = location_find_by_name(locations, name);
    } else {
        puts("Choose 1 to search by ID or 2 to search by name.");
        return true;
    }

    if (location == NULL) {
        puts("No matching location was found.");
    } else {
        printf("Found: %s (ID %d).\n", location->name, location->id);
    }
    return true;
}

static bool connect_locations_action(const LocationList *locations,
                                     Graph *graph)
{
    int id_a;
    int id_b;
    int metres;
    InputResult input_result;
    GraphStatus status;

    input_result = read_integer("Enter the first location ID: ", &id_a);
    if (input_result == INPUT_RESULT_EOF) {
        return false;
    }
    if (input_result != INPUT_RESULT_OK) {
        return true;
    }
    input_result = read_integer("Enter the second location ID: ", &id_b);
    if (input_result == INPUT_RESULT_EOF) {
        return false;
    }
    if (input_result != INPUT_RESULT_OK) {
        return true;
    }
    input_result = read_integer("Enter the walking distance in metres: ",
                                &metres);
    if (input_result == INPUT_RESULT_EOF) {
        return false;
    }
    if (input_result != INPUT_RESULT_OK) {
        return true;
    }

    status = graph_add_edge(graph, id_a, id_b, metres);
    if (status != GRAPH_STATUS_OK) {
        printf("Path was not added: %s\n", graph_status_message(status));
        return true;
    }

    printf("Added path: ");
    print_location_reference(locations, id_a);
    printf(" <-> ");
    print_location_reference(locations, id_b);
    printf(" (%d m).\n", metres);
    return true;
}

static void display_paths(const LocationList *locations, const Graph *graph)
{
    size_t first_index;
    size_t vertex_count = graph_vertex_count(graph);
    size_t path_count = 0;

    for (first_index = 0; first_index < vertex_count; first_index++) {
        int first_id;
        size_t second_index;

        if (!graph_get_vertex_id_at(graph, first_index, &first_id)) {
            continue;
        }
        for (second_index = first_index + 1; second_index < vertex_count;
             second_index++) {
            int second_id;
            int metres;

            if (!graph_get_vertex_id_at(graph, second_index, &second_id) ||
                !graph_get_edge_weight(graph, first_id, second_id, &metres)) {
                continue;
            }

            if (path_count == 0) {
                puts("Walking paths:");
            }
            printf("  ");
            print_location_reference(locations, first_id);
            printf(" <-> ");
            print_location_reference(locations, second_id);
            printf(": %d m\n", metres);
            path_count++;
        }
    }

    if (path_count == 0) {
        puts("No walking paths have been added.");
    }
}

static bool display_paths_action(const LocationList *locations,
                                 const Graph *graph)
{
    display_paths(locations, graph);
    return true;
}

static bool shortest_route_action(const LocationList *locations,
                                  const Graph *graph)
{
    int start_id;
    int destination_id;
    int route[GRAPH_MAX_VERTICES];
    int total_metres;
    size_t route_length;
    size_t index;
    InputResult input_result;
    GraphStatus status;

    input_result = read_integer("Enter the starting location ID: ", &start_id);
    if (input_result == INPUT_RESULT_EOF) {
        return false;
    }
    if (input_result != INPUT_RESULT_OK) {
        return true;
    }
    input_result = read_integer("Enter the destination location ID: ",
                                &destination_id);
    if (input_result == INPUT_RESULT_EOF) {
        return false;
    }
    if (input_result != INPUT_RESULT_OK) {
        return true;
    }

    status = graph_shortest_path(graph, start_id, destination_id, route,
                                 GRAPH_MAX_VERTICES, &route_length,
                                 &total_metres);
    if (status != GRAPH_STATUS_OK) {
        printf("Could not calculate route: %s\n",
               graph_status_message(status));
        return true;
    }

    printf("Shortest route: ");
    for (index = 0; index < route_length; index++) {
        if (index > 0) {
            printf(" -> ");
        }
        print_location_reference(locations, route[index]);
    }
    printf("\nTotal distance: %d m\n", total_metres);
    return true;
}

static bool bfs_action(const LocationList *locations, const Graph *graph)
{
    int start_id;
    int destination_id;
    int reachable[GRAPH_MAX_VERTICES];
    size_t reachable_count;
    size_t index;
    bool destination_reachable = false;
    InputResult input_result;
    GraphStatus status;

    input_result = read_integer("Enter the starting location ID: ", &start_id);
    if (input_result == INPUT_RESULT_EOF) {
        return false;
    }
    if (input_result != INPUT_RESULT_OK) {
        return true;
    }
    input_result = read_integer("Enter the destination location ID to check: ",
                                &destination_id);
    if (input_result == INPUT_RESULT_EOF) {
        return false;
    }
    if (input_result != INPUT_RESULT_OK) {
        return true;
    }

    status = graph_bfs(graph, start_id, reachable, GRAPH_MAX_VERTICES,
                       &reachable_count);
    if (status != GRAPH_STATUS_OK) {
        printf("Could not perform BFS: %s\n", graph_status_message(status));
        return true;
    }

    printf("Reachable locations in BFS order (%zu):\n", reachable_count);
    for (index = 0; index < reachable_count; index++) {
        printf("  ");
        print_location_reference(locations, reachable[index]);
        putchar('\n');
        if (reachable[index] == destination_id) {
            destination_reachable = true;
        }
    }

    if (!graph_has_vertex(graph, destination_id)) {
        printf("Destination ID %d is not in the graph.\n", destination_id);
    } else if (destination_reachable) {
        printf("Destination ");
        print_location_reference(locations, destination_id);
        printf(" is reachable from ");
        print_location_reference(locations, start_id);
        puts(".");
    } else {
        printf("Destination ");
        print_location_reference(locations, destination_id);
        printf(" is not reachable from ");
        print_location_reference(locations, start_id);
        puts(".");
    }
    return true;
}

static bool connectivity_action(const Graph *graph)
{
    bool connected;
    GraphStatus status = graph_is_connected(graph, &connected);

    if (status != GRAPH_STATUS_OK) {
        printf("Could not check connectivity: %s\n",
               graph_status_message(status));
    } else if (connected) {
        puts("The campus graph is connected.");
    } else {
        puts("The campus graph is not connected.");
    }
    return true;
}

static bool load_fictional_sample_data(LocationList *locations, Graph *graph)
{
    static const struct {
        int id;
        const char *name;
    } sample_locations[] = {
        {10, "Demo North Gate"},
        {73, "Demo Library"},
        {420, "Demo Cafeteria"},
        {501, "Demo Science Block"},
        {900, "Demo Sports Centre"}
    };
    static const struct {
        int id_a;
        int id_b;
        int metres;
    } sample_paths[] = {
        {10, 73, 120},
        {73, 420, 170},
        {10, 501, 260},
        {501, 420, 80},
        {73, 501, 150},
        {420, 900, 220},
        {501, 900, 350}
    };
    LocationList temporary_locations;
    Graph temporary_graph;
    const char *failure_message = NULL;
    size_t index;

    if (locations->count != 0 || graph_vertex_count(graph) != 0) {
        puts("Fictional sample data can only be loaded into an empty map.");
        return true;
    }

    location_list_init(&temporary_locations);
    graph_init(&temporary_graph);

    for (index = 0;
        index < sizeof(sample_locations) / sizeof(sample_locations[0]);
        index++) {
        GraphStatus graph_status =
            graph_add_vertex(&temporary_graph, sample_locations[index].id);
        LocationStatus location_status;

        if (graph_status != GRAPH_STATUS_OK) {
            failure_message = graph_status_message(graph_status);
            goto failure;
        }

        location_status = location_list_add(&temporary_locations,
                                            sample_locations[index].id,
                                            sample_locations[index].name);
        if (location_status != LOCATION_STATUS_OK) {
            (void)graph_remove_vertex(&temporary_graph,
                                      sample_locations[index].id);
            failure_message = location_status_message(location_status);
            goto failure;
        }
    }

    for (index = 0; index < sizeof(sample_paths) / sizeof(sample_paths[0]);
         index++) {
        GraphStatus status =
            graph_add_edge(&temporary_graph, sample_paths[index].id_a,
                           sample_paths[index].id_b,
                           sample_paths[index].metres);
        if (status != GRAPH_STATUS_OK) {
            failure_message = graph_status_message(status);
            goto failure;
        }
    }

    *locations = temporary_locations;
    *graph = temporary_graph;
    puts("Loaded five fictional demo locations and seven fictional paths.");
    puts("These labels and distances are development examples, not a real campus map.");
    return true;

failure:
    location_list_destroy(&temporary_locations);
    printf("Sample data was not loaded: %s\n", failure_message);
    return true;
}

static void display_menu(void)
{
    puts("\n=== Smart Campus Navigation System ===");
    puts("1. Add campus location");
    puts("2. Display all campus locations");
    puts("3. Search for a location");
    puts("4. Connect two locations");
    puts("5. Display connected paths");
    puts("6. Find the shortest route using Dijkstra");
    puts("7. Explore reachable locations using BFS");
    puts("8. Check graph connectivity");
    puts("9. Load fictional sample data");
    puts("10. Exit");
}

int main(void)
{
    LocationList locations;
    Graph graph;
    bool running = true;

    location_list_init(&locations);
    graph_init(&graph);

    while (running) {
        int choice;
        InputResult input_result;

        display_menu();
        input_result = read_integer("Choose an option: ", &choice);
        if (input_result == INPUT_RESULT_EOF) {
            puts("\nInput ended; exiting safely.");
            break;
        }
        if (input_result != INPUT_RESULT_OK) {
            puts("Please enter a valid menu number.");
            continue;
        }

        switch (choice) {
        case 1:
            running = add_location_action(&locations, &graph);
            break;
        case 2:
            location_list_display(&locations);
            break;
        case 3:
            running = search_location_action(&locations);
            break;
        case 4:
            running = connect_locations_action(&locations, &graph);
            break;
        case 5:
            (void)display_paths_action(&locations, &graph);
            break;
        case 6:
            running = shortest_route_action(&locations, &graph);
            break;
        case 7:
            running = bfs_action(&locations, &graph);
            break;
        case 8:
            (void)connectivity_action(&graph);
            break;
        case 9:
            (void)load_fictional_sample_data(&locations, &graph);
            break;
        case 10:
            running = false;
            puts("Goodbye.");
            break;
        default:
            puts("Invalid menu option. Choose a number from 1 to 10.");
            break;
        }

        if (!running && choice != 10) {
            puts("\nInput ended; exiting safely.");
        }
    }

    location_list_destroy(&locations);
    return EXIT_SUCCESS;
}
