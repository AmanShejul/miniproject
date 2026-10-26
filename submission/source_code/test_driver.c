#include "locations.h"
#include "graph.h"
#include "queue.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "FAIL at %s:%d: %s\n", __FILE__, __LINE__,       \
                    #condition);                                                \
            return EXIT_FAILURE;                                                \
        }                                                                       \
    } while (0)

static int test_location_module(void)
{
    LocationList locations;
    const Location *found;
    char exact_length_name[LOCATION_NAME_CAPACITY];
    size_t index;

    location_list_init(&locations);
    CHECK(locations.head == NULL);
    CHECK(locations.tail == NULL);
    CHECK(locations.count == 0);

    CHECK(location_list_add(&locations, 10, "North Gate") ==
          LOCATION_STATUS_OK);
    CHECK(location_list_add(&locations, 73, "Library") ==
          LOCATION_STATUS_OK);
    CHECK(locations.count == 2);

    CHECK(location_list_add(&locations, 10, "New Building") ==
          LOCATION_STATUS_DUPLICATE_ID);
    CHECK(location_list_add(&locations, 420, "lIbRaRy") ==
          LOCATION_STATUS_DUPLICATE_NAME);
    CHECK(location_list_add(&locations, 0, "Invalid ID") ==
          LOCATION_STATUS_INVALID_ID);
    CHECK(location_list_add(&locations, -5, "Negative ID") ==
          LOCATION_STATUS_INVALID_ID);
    CHECK(location_list_add(NULL, 420, "Null list") ==
          LOCATION_STATUS_INVALID_ARGUMENT);
    CHECK(location_list_add(&locations, 420, NULL) ==
          LOCATION_STATUS_INVALID_ARGUMENT);
    CHECK(location_list_add(&locations, 420, "   ") ==
          LOCATION_STATUS_EMPTY_NAME);
    CHECK(location_list_add(&locations, 420,
                            "This location name is deliberately longer "
                            "than sixty-three characters") ==
          LOCATION_STATUS_NAME_TOO_LONG);
    CHECK(locations.count == 2);

    for (index = 0; index < LOCATION_NAME_MAX; index++) {
        exact_length_name[index] = 'X';
    }
    exact_length_name[LOCATION_NAME_MAX] = '\0';
    CHECK(location_list_add(&locations, 420, exact_length_name) ==
          LOCATION_STATUS_OK);
    CHECK(strlen(locations.tail->name) == LOCATION_NAME_MAX);
    CHECK(locations.tail->id == 420);
    CHECK(locations.tail->next == NULL);
    CHECK(locations.count == 3);

    found = location_find_by_id(&locations, 73);
    CHECK(found != NULL);
    CHECK(found->id == 73);
    CHECK(found->name[0] == 'L');
    CHECK(location_find_by_id(&locations, 999) == NULL);

    found = location_find_by_name(&locations, "  nOrTh gAtE ");
    CHECK(found != NULL);
    CHECK(found->id == 10);
    CHECK(location_find_by_name(&locations, "Unlisted Hall") == NULL);

    location_list_destroy(&locations);
    CHECK(locations.head == NULL);
    CHECK(locations.tail == NULL);
    CHECK(locations.count == 0);

    /* Destroying an empty list is safe and leaves it reusable. */
    location_list_destroy(&locations);
    CHECK(location_list_add(&locations, 420, "Auditorium") ==
          LOCATION_STATUS_OK);
    location_list_destroy(&locations);
    CHECK(location_find_by_id(NULL, 10) == NULL);
    CHECK(location_find_by_name(NULL, "North Gate") == NULL);

    return EXIT_SUCCESS;
}

static int test_queue_module(void)
{
    Queue queue = {0};
    int value;

    CHECK(!queue_init(NULL, 3));
    CHECK(queue_is_empty(NULL));
    CHECK(!queue_is_full(NULL));
    CHECK(!queue_enqueue(NULL, 1));
    CHECK(!queue_dequeue(NULL, &value));
    queue_destroy(NULL);

    CHECK(queue_init(&queue, 3));
    CHECK(queue_is_empty(&queue));
    CHECK(!queue_is_full(&queue));
    CHECK(!queue_dequeue(&queue, &value));

    CHECK(queue_enqueue(&queue, 11));
    CHECK(queue_enqueue(&queue, 22));
    CHECK(queue_enqueue(&queue, 33));
    CHECK(queue_is_full(&queue));
    CHECK(!queue_enqueue(&queue, 44));

    CHECK(queue_dequeue(&queue, &value) && value == 11);
    CHECK(queue_enqueue(&queue, 44)); /* Wrap around to the start of the array. */
    CHECK(queue_dequeue(&queue, &value) && value == 22);
    CHECK(queue_dequeue(&queue, &value) && value == 33);
    CHECK(queue_dequeue(&queue, &value) && value == 44);
    CHECK(queue_is_empty(&queue));
    CHECK(!queue_dequeue(&queue, &value));

    /* Repeated single-item cycles exercise wraparound many times. */
    {
        int cycle;
        for (cycle = 0; cycle < 100; cycle++) {
            CHECK(queue_enqueue(&queue, cycle));
            CHECK(queue_dequeue(&queue, &value));
            CHECK(value == cycle);
        }
    }
    CHECK(!queue_dequeue(&queue, NULL));

    queue_destroy(&queue);
    CHECK(queue.items == NULL);
    CHECK(queue.capacity == 0);
    CHECK(!queue_init(&queue, 0));
    CHECK(!queue_init(&queue, SIZE_MAX));
    return EXIT_SUCCESS;
}

static int test_graph_module(void)
{
    Graph graph;
    Graph direct_graph;
    Graph overflow_graph;
    Graph full_graph;
    int route[GRAPH_MAX_VERTICES];
    int reachable[GRAPH_MAX_VERTICES];
    int distance;
    int edge_weight;
    int id;
    size_t route_length;
    size_t reachable_count;
    size_t index;
    bool connected;

    graph_init(&graph);
    CHECK(graph_vertex_count(&graph) == 0);
    CHECK(graph_add_vertex(&graph, 10) == GRAPH_STATUS_OK);
    CHECK(graph_add_vertex(&graph, 73) == GRAPH_STATUS_OK);
    CHECK(graph_add_vertex(&graph, 420) == GRAPH_STATUS_OK);
    CHECK(graph_add_vertex(&graph, 501) == GRAPH_STATUS_OK);
    CHECK(graph_add_vertex(&graph, 900) == GRAPH_STATUS_OK);
    CHECK(graph_add_vertex(&graph, 73) == GRAPH_STATUS_DUPLICATE_VERTEX);
    CHECK(graph_add_vertex(&graph, 0) == GRAPH_STATUS_INVALID_ID);
    CHECK(graph_add_vertex(&graph, -10) == GRAPH_STATUS_INVALID_ID);
    CHECK(graph_add_vertex(NULL, 1) == GRAPH_STATUS_INVALID_ARGUMENT);

    CHECK(graph_add_edge(&graph, 10, 73, 120) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&graph, 73, 420, 170) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&graph, 10, 501, 260) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&graph, 501, 420, 80) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&graph, 73, 501, 150) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&graph, 10, 420, 400) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&graph, 10, 73, 130) == GRAPH_STATUS_DUPLICATE_EDGE);
    CHECK(graph_add_edge(&graph, 10, 10, 100) ==
          GRAPH_STATUS_SELF_CONNECTION);
    CHECK(graph_add_edge(&graph, 10, 420, 0) ==
          GRAPH_STATUS_INVALID_DISTANCE);
    CHECK(graph_add_edge(&graph, 10, 420, -1) ==
          GRAPH_STATUS_INVALID_DISTANCE);
    CHECK(graph_add_edge(&graph, 10, 999, 10) ==
          GRAPH_STATUS_VERTEX_NOT_FOUND);
    CHECK(graph_get_edge_weight(&graph, 73, 10, &edge_weight));
    CHECK(edge_weight == 120); /* An undirected edge works in both directions. */
    CHECK(graph_get_edge_weight(&graph, 10, 73, &edge_weight));
    CHECK(edge_weight == 120);

    CHECK(graph_shortest_path(&graph, 10, 420, route,
                              GRAPH_MAX_VERTICES, &route_length,
                              &distance) == GRAPH_STATUS_OK);
    CHECK(route_length == 3);
    CHECK(route[0] == 10 && route[1] == 73 && route[2] == 420);
    CHECK(distance == 290);
    /* The 400 m direct edge loses to the 290 m indirect route. */
    CHECK(graph_shortest_path(&graph, 10, 420, route, 1, &route_length,
                              &distance) == GRAPH_STATUS_BUFFER_TOO_SMALL);
    CHECK(route_length == 3);
    CHECK(graph_shortest_path(&graph, 10, 10, route, GRAPH_MAX_VERTICES,
                              &route_length, &distance) == GRAPH_STATUS_OK);
    CHECK(route_length == 1 && route[0] == 10 && distance == 0);
    CHECK(graph_shortest_path(&graph, 10, 900, route, GRAPH_MAX_VERTICES,
                              &route_length, &distance) == GRAPH_STATUS_NO_ROUTE);
    CHECK(route_length == 0 && distance == 0);
    CHECK(graph_shortest_path(&graph, 999, 10, route, GRAPH_MAX_VERTICES,
                              &route_length, &distance) ==
          GRAPH_STATUS_VERTEX_NOT_FOUND);
    CHECK(graph_shortest_path(&graph, 10, 999, route, GRAPH_MAX_VERTICES,
                              &route_length, &distance) ==
          GRAPH_STATUS_VERTEX_NOT_FOUND);
    CHECK(graph_shortest_path(&graph, -1, 10, route, GRAPH_MAX_VERTICES,
                              &route_length, &distance) ==
          GRAPH_STATUS_INVALID_ID);

    CHECK(graph_bfs(&graph, 10, reachable, GRAPH_MAX_VERTICES,
                    &reachable_count) == GRAPH_STATUS_OK);
    CHECK(reachable_count == 4);
    CHECK(reachable[0] == 10 && reachable[1] == 73 &&
          reachable[2] == 420 && reachable[3] == 501);
    CHECK(graph_bfs(&graph, 10, reachable, 1, &reachable_count) ==
          GRAPH_STATUS_BUFFER_TOO_SMALL);
    CHECK(reachable_count == 4);
    CHECK(graph_bfs(&graph, 999, reachable, GRAPH_MAX_VERTICES,
                    &reachable_count) == GRAPH_STATUS_VERTEX_NOT_FOUND);
    CHECK(graph_bfs(&graph, -1, reachable, GRAPH_MAX_VERTICES,
                    &reachable_count) == GRAPH_STATUS_INVALID_ID);
    CHECK(graph_bfs(NULL, 10, reachable, GRAPH_MAX_VERTICES,
                    &reachable_count) == GRAPH_STATUS_INVALID_ARGUMENT);
    CHECK(graph_bfs(&graph, 10, NULL, GRAPH_MAX_VERTICES,
                    &reachable_count) == GRAPH_STATUS_INVALID_ARGUMENT);
    CHECK(graph_is_connected(&graph, &connected) == GRAPH_STATUS_OK);
    CHECK(!connected);

    CHECK(graph_add_edge(&graph, 420, 900, 220) == GRAPH_STATUS_OK);
    CHECK(graph_is_connected(&graph, &connected) == GRAPH_STATUS_OK);
    CHECK(connected);
    CHECK(graph_bfs(&graph, 900, reachable, GRAPH_MAX_VERTICES,
                    &reachable_count) == GRAPH_STATUS_OK);
    CHECK(reachable_count == 5);
    CHECK(reachable[0] == 900);

    CHECK(graph_remove_vertex(&graph, 73) == GRAPH_STATUS_OK);
    CHECK(!graph_has_vertex(&graph, 73));
    CHECK(graph_vertex_count(&graph) == 4);
    CHECK(graph_get_edge_weight(&graph, 10, 420, &edge_weight));
    CHECK(edge_weight == 400);
    CHECK(graph_get_edge_weight(&graph, 10, 501, &edge_weight));
    CHECK(edge_weight == 260);
    CHECK(graph_get_vertex_id_at(&graph, 1, &id));
    CHECK(id == 420);

    graph_init(&overflow_graph);
    CHECK(graph_add_vertex(&overflow_graph, 1) == GRAPH_STATUS_OK);
    CHECK(graph_add_vertex(&overflow_graph, 2) == GRAPH_STATUS_OK);
    CHECK(graph_add_vertex(&overflow_graph, 3) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&overflow_graph, 1, 2, INT_MAX) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&overflow_graph, 2, 3, INT_MAX) == GRAPH_STATUS_OK);
    CHECK(graph_shortest_path(&overflow_graph, 1, 3, route,
                              GRAPH_MAX_VERTICES, &route_length,
                              &distance) == GRAPH_STATUS_DISTANCE_OVERFLOW);

    graph_init(&full_graph);
    for (index = 0; index < GRAPH_MAX_VERTICES - 1; index++) {
        CHECK(graph_add_vertex(&full_graph, 2000 + (int)index) ==
              GRAPH_STATUS_OK);
    }
    CHECK(graph_vertex_count(&full_graph) == 99);
    CHECK(graph_add_vertex(&full_graph, 9998) == GRAPH_STATUS_OK);
    CHECK(graph_vertex_count(&full_graph) == GRAPH_MAX_VERTICES);
    CHECK(graph_add_vertex(&full_graph, 9999) ==
          GRAPH_STATUS_CAPACITY_REACHED);

    graph_init(&direct_graph);
    CHECK(graph_add_vertex(&direct_graph, 10) == GRAPH_STATUS_OK);
    CHECK(graph_add_vertex(&direct_graph, 73) == GRAPH_STATUS_OK);
    CHECK(graph_add_vertex(&direct_graph, 420) == GRAPH_STATUS_OK);
    CHECK(graph_add_vertex(&direct_graph, 501) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&direct_graph, 10, 420, 100) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&direct_graph, 10, 73, 30) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&direct_graph, 73, 420, 100) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&direct_graph, 10, 501, 40) == GRAPH_STATUS_OK);
    CHECK(graph_add_edge(&direct_graph, 501, 420, 100) == GRAPH_STATUS_OK);
    CHECK(graph_shortest_path(&direct_graph, 10, 420, route,
                              GRAPH_MAX_VERTICES, &route_length,
                              &distance) == GRAPH_STATUS_OK);
    CHECK(route_length == 2 && route[0] == 10 && route[1] == 420);
    CHECK(distance == 100);

    graph_init(&graph);
    CHECK(graph_is_connected(&graph, &connected) == GRAPH_STATUS_OK);
    CHECK(!connected);
    CHECK(graph_add_vertex(&graph, 42) == GRAPH_STATUS_OK);
    CHECK(graph_is_connected(&graph, &connected) == GRAPH_STATUS_OK);
    CHECK(connected);

    return EXIT_SUCCESS;
}

int main(void)
{
    if (test_location_module() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (test_queue_module() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (test_graph_module() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    puts("Location, queue, and graph module tests passed.");
    return EXIT_SUCCESS;
}
