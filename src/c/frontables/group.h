#pragma once

#include <pebble.h>
#include "frontable.h"
#include "frontable_list.h"

#define GROUP_NAME_LENGTH 32

struct Group;

/// @brief A struct that describes a group of frontables in a plural system
typedef struct Group {
    struct Group* parent;
    char name[GROUP_NAME_LENGTH + 1];
    GColor color;
    FrontableList* frontables;
} Group;

/// @brief Creates a new group on the heap
/// @param name Name of group, will be copied from pointer
/// @param color Color of group
/// @param parent Pointer to parent group
/// @return A pointer to a new group on the heap
Group* group_create(const char* name, GColor color, Group* parent);

/// @brief Destroys a group, freeing memory from the heap
/// @param group Group to destroy
void group_destroy(Group* group);
