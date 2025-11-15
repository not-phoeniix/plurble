#pragma once

#include <pebble.h>
#include "frontable.h"
#include "frontable_list.h"

#define GROUP_NAME_LENGTH 32

struct Group;

typedef struct Group {
    struct Group* parent;
    char name[GROUP_NAME_LENGTH + 1];
    GColor color;
    FrontableList* members;
} Group;
