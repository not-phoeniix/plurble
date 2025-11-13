#pragma once

#include <pebble.h>
#include "frontable.h"
#include "frontable_list.h"

#define GROUP_NAME_LENGTH 32

// dude i gotta make this a tree . wow. how the fuck am i gonna do that

struct Group;

typedef struct Group {
    struct Group* parent;
    char name[GROUP_NAME_LENGTH + 1];
    GColor color;
    FrontableList members;
} Group;
