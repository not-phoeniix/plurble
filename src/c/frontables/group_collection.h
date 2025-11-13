#pragma once

#include "group.h"

#define GROUP_LIST_MAX_COUNT 16

typedef struct {
    Group groups[GROUP_LIST_MAX_COUNT];
    uint16_t num_stored;
} GroupCollection;
