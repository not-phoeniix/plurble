#include "custom_fronts_menu.h"
#include <pebble.h>
#include "frontable_menu.h"
#include "../data/frontable_cache.h"
#include "../data/config.h"

static FrontableMenu* menu = NULL;
static Group group;

static void select(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    frontable_menu_select(menu, cell_index);
}

static void draw_cell(FrontableMenu* menu, GContext* ctx, const Layer* cell_layer, Frontable* selected_frontable, Group* selected_group) {
    if (selected_frontable == NULL) {
        return;
    }

    bool compact = settings_get()->compact_member_list;

    char* label = NULL;
    if (!compact && settings_get()->custom_front_text[0] != '\0') {
        label = settings_get()->custom_front_text;
    }

    frontable_menu_draw_cell_custom(
        menu,
        ctx,
        cell_layer,
        selected_frontable->name,
        label,
        NULL,
        frontable_get_color(selected_frontable)
    );
}

void custom_fronts_menu_push() {
    if (menu == NULL) {
        MemberMenuCallbacks callbacks = {
            .draw_row = draw_cell,
            .select = select,
            .window_load = NULL,
            .window_unload = NULL
        };

        group.color = settings_get()->background_color;
        group.frontables = cache_get_custom_fronts();
        strcpy(group.name, "Custom Fronts");
        group.parent = NULL;
        menu = frontable_menu_create(callbacks, &group);
    }

    frontable_menu_window_push(menu, false, true);
}

void custom_fronts_menu_deinit() {
    if (menu != NULL) {
        frontable_menu_destroy(menu);
        menu = NULL;
    }
}

void custom_fronts_menu_update_colors() {
    group.color = settings_get()->background_color;

    if (menu != NULL) {
        frontable_menu_update_colors(menu);
    }
}
