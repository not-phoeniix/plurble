#pragma once

#include <pebble.h>
#include "frontable.h"

/// @brief A struct representing a dynamic array of frontables
typedef struct {
    Frontable** frontables;
    uint16_t size;
    uint16_t num_stored;
} FrontableList;

/// @brief Creates a new frontable list on the heap
/// @return Pointer to new frontable list on heap
FrontableList* frontable_list_create();

/// @brief Destroys a frontable list, freeing memory in the process
/// @param list Frontable list to destroy
void frontable_list_destroy(FrontableList* list);

/// @brief Adds a frontable to the end of a frontable list
/// @param to_add Frontable to add
/// @param list List to add to
void frontable_list_add(Frontable* to_add, FrontableList* list);

/// @brief Clears a frontable list, does not free memory of contained frontables
/// @param list List to clear
void frontable_list_clear(FrontableList* list);

/// @brief Clears a frontable list and frees memory of all contained frontables
/// @param list List to clear
void frontable_list_deep_clear(FrontableList* list);
