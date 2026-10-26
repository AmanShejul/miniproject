#ifndef LOCATIONS_H
#define LOCATIONS_H

#include <stddef.h>

#define LOCATION_NAME_MAX 63
#define LOCATION_NAME_CAPACITY (LOCATION_NAME_MAX + 1)

typedef struct Location {
    int id;
    char name[LOCATION_NAME_CAPACITY];
    struct Location *next;
} Location;

typedef struct {
    Location *head;
    Location *tail;
    size_t count;
} LocationList;

typedef enum {
    LOCATION_STATUS_OK = 0,
    LOCATION_STATUS_INVALID_ARGUMENT,
    LOCATION_STATUS_INVALID_ID,
    LOCATION_STATUS_EMPTY_NAME,
    LOCATION_STATUS_NAME_TOO_LONG,
    LOCATION_STATUS_DUPLICATE_ID,
    LOCATION_STATUS_DUPLICATE_NAME,
    LOCATION_STATUS_OUT_OF_MEMORY
} LocationStatus;

void location_list_init(LocationList *list);
LocationStatus location_list_add(LocationList *list, int id, const char *name);
const Location *location_find_by_id(const LocationList *list, int id);
const Location *location_find_by_name(const LocationList *list,
                                      const char *name);
void location_list_display(const LocationList *list);
void location_list_destroy(LocationList *list);
const char *location_status_message(LocationStatus status);

#endif
