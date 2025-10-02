#include "frontable_cache.h"
#include "../tools/string_tools.h"
#include "../menus/main_menu.h"
#include "../menus/fronters_menu.h"

#define MAX_CACHED_FRONTABLES 256

static FrontableList members;
static FrontableList custom_fronts;
static FrontableList current_fronters;

FrontableList* cache_get_members() { return &members; }
FrontableList* cache_get_custom_fronts() { return &custom_fronts; }
FrontableList* cache_get_current_fronters() { return &current_fronters; }
Frontable* cache_get_first_fronter() { return current_fronters.frontables[0]; }

void cache_add_frontable(const Frontable* frontable) {
    if (frontable_get_is_custom(frontable)) {
        frontable_list_add(frontable, &custom_fronts);
    } else {
        frontable_list_add(frontable, &members);
    }
}

void cache_clear_frontables() {
    frontable_list_deep_clear(&members);
    frontable_list_deep_clear(&custom_fronts);
}

void cache_add_current_fronter(uint32_t frontable_hash) {
    Frontable* frontable = NULL;

    //! using just iteration to search isn't the best idea...
    //!   for the future, since we have hashes for each frontable:
    //!   make a hash map maybe !!!!

    for (uint16_t i = 0; i < members.num_stored; i++) {
        Frontable* f = members.frontables[i];

        if (f->hash == frontable_hash) {
            frontable = f;
            break;
        }
    }

    if (frontable == NULL) {
        for (uint16_t i = 0; i < custom_fronts.num_stored; i++) {
            Frontable* f = custom_fronts.frontables[i];

            if (f->hash == frontable_hash) {
                frontable = f;
                break;
            }
        }
    }

    if (frontable != NULL) {
        frontable_list_add(frontable, &current_fronters);
    }
}

void cache_clear_current_fronters() {
    frontable_list_clear(&current_fronters);
}

void frontable_cache_deinit() {
    cache_clear_current_fronters();
    cache_clear_frontables();
}
