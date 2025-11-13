#include "members_menu.h"
#include <pebble.h>
#include "frontable_menu.h"
#include "../data/frontable_cache.h"

static FrontableMenu* menu = NULL;

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    frontable_menu_draw_cell(menu, ctx, cell_layer, cell_index);
}

static void select(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    frontable_menu_select(menu, cell_index);
}

void members_menu_push() {
    if (menu == NULL) {
        MemberMenuCallbacks callbacks = {
            .draw_row = draw_row,
            .select = select,
            .window_load = NULL,
            .window_unload = NULL
        };

        Group groups[] = {
            {.color = GColorRed, .name = "RedGroop", .parent = NULL},
            {.color = GColorShockingPink, .name = "Other :(", .parent = NULL}
        };

        menu = frontable_menu_create(callbacks, cache_get_members(), groups, 2, "Member List");
    }

    frontable_menu_window_push(menu);
}

void members_menu_deinit() {
    if (menu != NULL) {
        frontable_menu_destroy(menu);
        menu = NULL;
    }
}

void members_menu_update_colors() {
    if (menu != NULL) {
        frontable_menu_update_colors(menu);
    }
}
