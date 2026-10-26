#include "locations.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Copy a trimmed name into a fixed-size buffer. */
static LocationStatus normalize_name(const char *name,
                                     char output[LOCATION_NAME_CAPACITY])
{
    const char *start;
    const char *end;
    size_t length;

    if (name == NULL || output == NULL) {
        return LOCATION_STATUS_INVALID_ARGUMENT;
    }

    start = name;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1])) {
        end--;
    }

    length = (size_t)(end - start);
    if (length == 0) {
        return LOCATION_STATUS_EMPTY_NAME;
    }
    if (length > LOCATION_NAME_MAX) {
        return LOCATION_STATUS_NAME_TOO_LONG;
    }

    memcpy(output, start, length);
    output[length] = '\0';
    return LOCATION_STATUS_OK;
}

/* Compare two names without treating ASCII letter case as significant. */
static int names_equal_case_insensitive(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        unsigned char left_char = (unsigned char)*left;
        unsigned char right_char = (unsigned char)*right;

        if (tolower(left_char) != tolower(right_char)) {
            return 0;
        }
        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

void location_list_init(LocationList *list)
{
    if (list == NULL) {
        return;
    }

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

LocationStatus location_list_add(LocationList *list, int id, const char *name)
{
    char normalized_name[LOCATION_NAME_CAPACITY];
    LocationStatus status;
    Location *current;
    Location *new_location;

    if (list == NULL || name == NULL) {
        return LOCATION_STATUS_INVALID_ARGUMENT;
    }
    if (id <= 0) {
        return LOCATION_STATUS_INVALID_ID;
    }

    status = normalize_name(name, normalized_name);
    if (status != LOCATION_STATUS_OK) {
        return status;
    }

    for (current = list->head; current != NULL; current = current->next) {
        if (current->id == id) {
            return LOCATION_STATUS_DUPLICATE_ID;
        }
        if (names_equal_case_insensitive(current->name, normalized_name)) {
            return LOCATION_STATUS_DUPLICATE_NAME;
        }
    }

    new_location = malloc(sizeof(*new_location));
    if (new_location == NULL) {
        return LOCATION_STATUS_OUT_OF_MEMORY;
    }

    new_location->id = id;
    memcpy(new_location->name, normalized_name,
           strlen(normalized_name) + 1);
    new_location->next = NULL;

    if (list->tail == NULL) {
        list->head = new_location;
    } else {
        list->tail->next = new_location;
    }
    list->tail = new_location;
    list->count++;

    return LOCATION_STATUS_OK;
}

const Location *location_find_by_id(const LocationList *list, int id)
{
    const Location *current;

    if (list == NULL || id <= 0) {
        return NULL;
    }

    for (current = list->head; current != NULL; current = current->next) {
        if (current->id == id) {
            return current;
        }
    }

    return NULL;
}

const Location *location_find_by_name(const LocationList *list,
                                      const char *name)
{
    char normalized_name[LOCATION_NAME_CAPACITY];
    const Location *current;

    if (list == NULL || name == NULL ||
        normalize_name(name, normalized_name) != LOCATION_STATUS_OK) {
        return NULL;
    }

    for (current = list->head; current != NULL; current = current->next) {
        if (names_equal_case_insensitive(current->name, normalized_name)) {
            return current;
        }
    }

    return NULL;
}

void location_list_display(const LocationList *list)
{
    const Location *current;

    if (list == NULL) {
        puts("Location list is unavailable.");
        return;
    }
    if (list->head == NULL) {
        puts("No campus locations have been added.");
        return;
    }

    puts("Campus locations:");
    for (current = list->head; current != NULL; current = current->next) {
        printf("  ID: %d | Name: %s\n", current->id, current->name);
    }
}

void location_list_destroy(LocationList *list)
{
    Location *current;

    if (list == NULL) {
        return;
    }

    current = list->head;
    while (current != NULL) {
        Location *next = current->next;
        free(current);
        current = next;
    }

    location_list_init(list);
}

const char *location_status_message(LocationStatus status)
{
    switch (status) {
    case LOCATION_STATUS_OK:
        return "Location added successfully.";
    case LOCATION_STATUS_INVALID_ARGUMENT:
        return "Invalid argument supplied.";
    case LOCATION_STATUS_INVALID_ID:
        return "Location ID must be a positive integer.";
    case LOCATION_STATUS_EMPTY_NAME:
        return "Location name cannot be empty.";
    case LOCATION_STATUS_NAME_TOO_LONG:
        return "Location name must be at most 63 characters.";
    case LOCATION_STATUS_DUPLICATE_ID:
        return "That location ID is already in use.";
    case LOCATION_STATUS_DUPLICATE_NAME:
        return "That location name is already in use.";
    case LOCATION_STATUS_OUT_OF_MEMORY:
        return "Memory allocation failed; no location was added.";
    default:
        return "Unknown location status.";
    }
}
