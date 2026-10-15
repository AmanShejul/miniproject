#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>
#include <stddef.h>

#define GRAPH_MAX_VERTICES 100

typedef struct {
    int location_ids[GRAPH_MAX_VERTICES];
    int weights[GRAPH_MAX_VERTICES][GRAPH_MAX_VERTICES];
    size_t vertex_count;
} Graph;

typedef enum {
    GRAPH_STATUS_OK = 0,
    GRAPH_STATUS_INVALID_ARGUMENT,
    GRAPH_STATUS_INVALID_ID,
    GRAPH_STATUS_DUPLICATE_VERTEX,
    GRAPH_STATUS_CAPACITY_REACHED,
    GRAPH_STATUS_VERTEX_NOT_FOUND,
    GRAPH_STATUS_INVALID_DISTANCE,
    GRAPH_STATUS_SELF_CONNECTION,
    GRAPH_STATUS_DUPLICATE_EDGE,
    GRAPH_STATUS_NO_ROUTE,
    GRAPH_STATUS_BUFFER_TOO_SMALL,
    GRAPH_STATUS_DISTANCE_OVERFLOW,
    GRAPH_STATUS_OUT_OF_MEMORY,
    GRAPH_STATUS_QUEUE_ERROR
} GraphStatus;

void graph_init(Graph *graph);
GraphStatus graph_add_vertex(Graph *graph, int location_id);
GraphStatus graph_remove_vertex(Graph *graph, int location_id);
GraphStatus graph_add_edge(Graph *graph, int id_a, int id_b, int metres);
size_t graph_vertex_count(const Graph *graph);
bool graph_has_vertex(const Graph *graph, int location_id);
bool graph_get_vertex_id_at(const Graph *graph, size_t index, int *location_id);
bool graph_get_edge_weight(const Graph *graph, int id_a, int id_b,
                           int *metres);
GraphStatus graph_shortest_path(const Graph *graph,
                                int start_id,
                                int destination_id,
                                int route_ids[],
                                size_t route_capacity,
                                size_t *route_length,
                                int *total_metres);
GraphStatus graph_bfs(const Graph *graph,
                      int start_id,
                      int reachable_ids[],
                      size_t reachable_capacity,
                      size_t *reachable_count);
GraphStatus graph_is_connected(const Graph *graph, bool *connected);
const char *graph_status_message(GraphStatus status);

#endif
