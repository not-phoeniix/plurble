#include "frontable_list.h"

static void double_size(FrontableList* list) {
    if (list->frontables == NULL) {
        list->size = 1;
        list->frontables = malloc(sizeof(Frontable*));
    } else {
        list->size *= 2;
        list->frontables = realloc(list->frontables, sizeof(Frontable*) * list->size);

        if (list->frontables == NULL) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "ERROR!!!!, Frontable list realloc failed!!!");
        }
    }
}

void frontable_list_add(const Frontable* to_add, FrontableList* list) {
    while (list->num_stored >= list->size) {
        double_size(list);
    }

    list->frontables[list->num_stored] = to_add;
    list->num_stored++;
}

void frontable_list_clear(FrontableList* list) {
    if (list->frontables != NULL) {
        free(list->frontables);
        list->frontables = NULL;
    }

    list->size = 0;
    list->num_stored = 0;
}

void frontable_list_deep_clear(FrontableList* list) {
    for (uint16_t i = 0; i < list->num_stored; i++) {
        free(list->frontables[i]);
    }

    frontable_list_clear(list);
}
