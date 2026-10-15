#include "graph.h"

#include "queue.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool find_vertex_index(const Graph *graph, int location_id,
                              size_t *index_out)
{
    size_t index;

    if (graph == NULL || index_out == NULL ||
        graph->vertex_count > GRAPH_MAX_VERTICES) {
        return false;
    }

    for (index = 0; index < graph->vertex_count; index++) {
        if (graph->location_ids[index] == location_id) {
            *index_out = index;
            return true;
        }
    }

    return false;
}

void graph_init(Graph *graph)
{
    if (graph == NULL) {
        return;
    }

    memset(graph, 0, sizeof(*graph));
}

GraphStatus graph_add_vertex(Graph *graph, int location_id)
{
    size_t unused_index;

    if (graph == NULL) {
        return GRAPH_STATUS_INVALID_ARGUMENT;
    }
    if (location_id <= 0) {
        return GRAPH_STATUS_INVALID_ID;
    }
    if (graph->vertex_count > GRAPH_MAX_VERTICES) {
        return GRAPH_STATUS_INVALID_ARGUMENT;
    }
    if (find_vertex_index(graph, location_id, &unused_index)) {
        return GRAPH_STATUS_DUPLICATE_VERTEX;
    }
    if (graph->vertex_count == GRAPH_MAX_VERTICES) {
        return GRAPH_STATUS_CAPACITY_REACHED;
    }

    graph->location_ids[graph->vertex_count] = location_id;
    graph->vertex_count++;
    return GRAPH_STATUS_OK;
}

GraphStatus graph_remove_vertex(Graph *graph, int location_id)
{
    size_t removed_index;
    size_t old_count;
    size_t row;
    size_t column;

    if (graph == NULL) {
        return GRAPH_STATUS_INVALID_ARGUMENT;
    }
    if (location_id <= 0) {
        return GRAPH_STATUS_INVALID_ID;
    }
    if (!find_vertex_index(graph, location_id, &removed_index)) {
        return GRAPH_STATUS_VERTEX_NOT_FOUND;
    }

    old_count = graph->vertex_count;
    for (row = removed_index; row + 1 < old_count; row++) {
        graph->location_ids[row] = graph->location_ids[row + 1];
        for (column = 0; column < old_count; column++) {
            graph->weights[row][column] = graph->weights[row + 1][column];
        }
    }

    for (row = 0; row + 1 < old_count; row++) {
        for (column = removed_index; column + 1 < old_count; column++) {
            graph->weights[row][column] = graph->weights[row][column + 1];
        }
    }

    graph->vertex_count--;
    graph->location_ids[graph->vertex_count] = 0;
    for (row = 0; row < old_count; row++) {
        graph->weights[graph->vertex_count][row] = 0;
        graph->weights[row][graph->vertex_count] = 0;
    }

    return GRAPH_STATUS_OK;
}

GraphStatus graph_add_edge(Graph *graph, int id_a, int id_b, int metres)
{
    size_t index_a;
    size_t index_b;

    if (graph == NULL) {
        return GRAPH_STATUS_INVALID_ARGUMENT;
    }
    if (id_a <= 0 || id_b <= 0) {
        return GRAPH_STATUS_INVALID_ID;
    }
    if (metres <= 0) {
        return GRAPH_STATUS_INVALID_DISTANCE;
    }
    if (id_a == id_b) {
        return GRAPH_STATUS_SELF_CONNECTION;
    }
    if (!find_vertex_index(graph, id_a, &index_a) ||
        !find_vertex_index(graph, id_b, &index_b)) {
        return GRAPH_STATUS_VERTEX_NOT_FOUND;
    }
    if (graph->weights[index_a][index_b] != 0) {
        return GRAPH_STATUS_DUPLICATE_EDGE;
    }

    graph->weights[index_a][index_b] = metres;
    graph->weights[index_b][index_a] = metres;
    return GRAPH_STATUS_OK;
}

size_t graph_vertex_count(const Graph *graph)
{
    if (graph == NULL || graph->vertex_count > GRAPH_MAX_VERTICES) {
        return 0;
    }
    return graph->vertex_count;
}

bool graph_has_vertex(const Graph *graph, int location_id)
{
    size_t unused_index;

    return location_id > 0 &&
           find_vertex_index(graph, location_id, &unused_index);
}

bool graph_get_vertex_id_at(const Graph *graph, size_t index, int *location_id)
{
    if (graph == NULL || location_id == NULL ||
        index >= graph->vertex_count ||
        graph->vertex_count > GRAPH_MAX_VERTICES) {
        return false;
    }

    *location_id = graph->location_ids[index];
    return true;
}

bool graph_get_edge_weight(const Graph *graph, int id_a, int id_b,
                           int *metres)
{
    size_t index_a;
    size_t index_b;

    if (graph == NULL || metres == NULL || id_a <= 0 || id_b <= 0 ||
        !find_vertex_index(graph, id_a, &index_a) ||
        !find_vertex_index(graph, id_b, &index_b)) {
        return false;
    }

    *metres = graph->weights[index_a][index_b];
    return *metres > 0;
}

GraphStatus graph_shortest_path(const Graph *graph,
                                int start_id,
                                int destination_id,
                                int route_ids[],
                                size_t route_capacity,
                                size_t *route_length,
                                int *total_metres)
{
    long long distances[GRAPH_MAX_VERTICES];
    size_t predecessors[GRAPH_MAX_VERTICES];
    size_t reverse_route[GRAPH_MAX_VERTICES];
    bool visited[GRAPH_MAX_VERTICES] = {false};
    size_t start_index;
    size_t destination_index;
    size_t route_size = 0;
    size_t index;

    if (graph == NULL || route_ids == NULL || route_length == NULL ||
        total_metres == NULL || graph->vertex_count > GRAPH_MAX_VERTICES) {
        return GRAPH_STATUS_INVALID_ARGUMENT;
    }
    *route_length = 0;
    *total_metres = 0;

    if (start_id <= 0 || destination_id <= 0) {
        return GRAPH_STATUS_INVALID_ID;
    }
    if (!find_vertex_index(graph, start_id, &start_index) ||
        !find_vertex_index(graph, destination_id, &destination_index)) {
        return GRAPH_STATUS_VERTEX_NOT_FOUND;
    }

    for (index = 0; index < graph->vertex_count; index++) {
        distances[index] = LLONG_MAX;
        predecessors[index] = SIZE_MAX;
    }
    distances[start_index] = 0;

    for (index = 0; index < graph->vertex_count; index++) {
        size_t current = SIZE_MAX;
        size_t candidate;

        for (candidate = 0; candidate < graph->vertex_count; candidate++) {
            if (!visited[candidate] && distances[candidate] != LLONG_MAX &&
                (current == SIZE_MAX ||
                 distances[candidate] < distances[current])) {
                current = candidate;
            }
        }

        if (current == SIZE_MAX) {
            break;
        }
        if (current == destination_index) {
            break;
        }
        visited[current] = true;

        for (candidate = 0; candidate < graph->vertex_count; candidate++) {
            int edge_weight = graph->weights[current][candidate];
            long long new_distance;

            if (edge_weight <= 0 || visited[candidate]) {
                continue;
            }
            if (distances[current] > LLONG_MAX - edge_weight) {
                return GRAPH_STATUS_DISTANCE_OVERFLOW;
            }

            new_distance = distances[current] + edge_weight;
            if (new_distance < distances[candidate]) {
                distances[candidate] = new_distance;
                predecessors[candidate] = current;
            }
        }
    }

    if (distances[destination_index] == LLONG_MAX) {
        return GRAPH_STATUS_NO_ROUTE;
    }
    if (distances[destination_index] > INT_MAX) {
        return GRAPH_STATUS_DISTANCE_OVERFLOW;
    }

    {
        size_t current = destination_index;
        while (current != SIZE_MAX && route_size < graph->vertex_count) {
            reverse_route[route_size++] = current;
            if (current == start_index) {
                break;
            }
            current = predecessors[current];
        }
    }

    if (route_size == 0 || reverse_route[route_size - 1] != start_index) {
        return GRAPH_STATUS_NO_ROUTE;
    }
    *route_length = route_size;
    if (route_capacity < route_size) {
        return GRAPH_STATUS_BUFFER_TOO_SMALL;
    }

    for (index = 0; index < route_size; index++) {
        route_ids[index] =
            graph->location_ids[reverse_route[route_size - index - 1]];
    }
    *total_metres = (int)distances[destination_index];
    return GRAPH_STATUS_OK;
}

GraphStatus graph_bfs(const Graph *graph,
                      int start_id,
                      int reachable_ids[],
                      size_t reachable_capacity,
                      size_t *reachable_count)
{
    bool visited[GRAPH_MAX_VERTICES] = {false};
    int order[GRAPH_MAX_VERTICES];
    size_t start_index;
    size_t order_count = 0;
    size_t index;
    Queue queue = {0};
    int current_value;

    if (graph == NULL || reachable_ids == NULL || reachable_count == NULL ||
        graph->vertex_count > GRAPH_MAX_VERTICES) {
        return GRAPH_STATUS_INVALID_ARGUMENT;
    }
    *reachable_count = 0;

    if (start_id <= 0) {
        return GRAPH_STATUS_INVALID_ID;
    }
    if (!find_vertex_index(graph, start_id, &start_index)) {
        return GRAPH_STATUS_VERTEX_NOT_FOUND;
    }
    if (!queue_init(&queue, graph->vertex_count)) {
        return GRAPH_STATUS_OUT_OF_MEMORY;
    }

    visited[start_index] = true;
    if (!queue_enqueue(&queue, (int)start_index)) {
        queue_destroy(&queue);
        return GRAPH_STATUS_QUEUE_ERROR;
    }

    while (!queue_is_empty(&queue)) {
        size_t current;
        size_t neighbor;

        if (!queue_dequeue(&queue, &current_value)) {
            queue_destroy(&queue);
            return GRAPH_STATUS_QUEUE_ERROR;
        }
        current = (size_t)current_value;
        order[order_count++] = graph->location_ids[current];

        for (neighbor = 0; neighbor < graph->vertex_count; neighbor++) {
            if (graph->weights[current][neighbor] > 0 && !visited[neighbor]) {
                visited[neighbor] = true;
                if (!queue_enqueue(&queue, (int)neighbor)) {
                    queue_destroy(&queue);
                    return GRAPH_STATUS_QUEUE_ERROR;
                }
            }
        }
    }

    queue_destroy(&queue);
    *reachable_count = order_count;
    if (reachable_capacity < order_count) {
        return GRAPH_STATUS_BUFFER_TOO_SMALL;
    }

    for (index = 0; index < order_count; index++) {
        reachable_ids[index] = order[index];
    }
    return GRAPH_STATUS_OK;
}

GraphStatus graph_is_connected(const Graph *graph, bool *connected)
{
    int reachable_ids[GRAPH_MAX_VERTICES];
    size_t reachable_count = 0;
    GraphStatus status;

    if (graph == NULL || connected == NULL ||
        graph->vertex_count > GRAPH_MAX_VERTICES) {
        return GRAPH_STATUS_INVALID_ARGUMENT;
    }

    *connected = false;
    if (graph->vertex_count == 0) {
        return GRAPH_STATUS_OK;
    }

    status = graph_bfs(graph, graph->location_ids[0], reachable_ids,
                       GRAPH_MAX_VERTICES, &reachable_count);
    if (status != GRAPH_STATUS_OK) {
        return status;
    }

    *connected = reachable_count == graph->vertex_count;
    return GRAPH_STATUS_OK;
}

const char *graph_status_message(GraphStatus status)
{
    switch (status) {
    case GRAPH_STATUS_OK:
        return "Operation completed successfully.";
    case GRAPH_STATUS_INVALID_ARGUMENT:
        return "Invalid graph argument or state.";
    case GRAPH_STATUS_INVALID_ID:
        return "Location IDs must be positive integers.";
    case GRAPH_STATUS_DUPLICATE_VERTEX:
        return "That location ID is already in the graph.";
    case GRAPH_STATUS_CAPACITY_REACHED:
        return "The graph has reached its 100-location limit.";
    case GRAPH_STATUS_VERTEX_NOT_FOUND:
        return "One or more location IDs are not in the graph.";
    case GRAPH_STATUS_INVALID_DISTANCE:
        return "Path distance must be a positive number of metres.";
    case GRAPH_STATUS_SELF_CONNECTION:
        return "A location cannot have a path to itself.";
    case GRAPH_STATUS_DUPLICATE_EDGE:
        return "A path already exists between those locations.";
    case GRAPH_STATUS_NO_ROUTE:
        return "No route connects those locations.";
    case GRAPH_STATUS_BUFFER_TOO_SMALL:
        return "The output array is too small for the result.";
    case GRAPH_STATUS_DISTANCE_OVERFLOW:
        return "The route distance exceeds the supported integer range.";
    case GRAPH_STATUS_OUT_OF_MEMORY:
        return "Memory allocation failed while creating the BFS queue.";
    case GRAPH_STATUS_QUEUE_ERROR:
        return "The BFS queue could not store or retrieve a vertex.";
    default:
        return "Unknown graph status.";
    }
}
