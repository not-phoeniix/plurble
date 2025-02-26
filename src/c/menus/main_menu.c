#include "main_menu.h"
#include "members_menu.h"
#include "../tools/string_tools.h"
#include "../config/config.h"

static Window* window = NULL;
static SimpleMenuLayer* simple_menu_layer = NULL;
static SimpleMenuItem items[2];
static SimpleMenuSection sections[1];
static bool members_loaded = false;

static void select(int index, void* context) {
    switch (index) {
        // "members" case
        case 0:
            if (members_loaded) {
                members_menu_push();
            }
            break;
        case 1:
            break;
    }
}

static void window_load() {
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    items[0] = (SimpleMenuItem) {
        .title = "Member List",
        .subtitle = "loading members...",
        .icon = NULL,
        .callback = select
    };

    items[1] = (SimpleMenuItem) {
        .title = "Fronters",
        .subtitle = "who's at front",
        .icon = NULL,
        .callback = select
    };

    sections[0] = (SimpleMenuSection) {
        .items = items,
        .num_items = 1,
        .title = "Member Management"
    };

    simple_menu_layer = simple_menu_layer_create(bounds, window, sections, 1, NULL);

    main_menu_update_colors();

    layer_add_child(window_layer, simple_menu_layer_get_layer(simple_menu_layer));
}

static void window_unload() {
    simple_menu_layer_destroy(simple_menu_layer);
    simple_menu_layer = NULL;
}

void main_menu_push() {
    if (window == NULL) {
        window = window_create();
        window_set_window_handlers(
            window,
            (WindowHandlers) {
                .load = window_load,
                .unload = window_unload
            }
        );
    }

    window_stack_push(window, true);
}

void main_menu_update_colors() {
    ClaySettings* settings = settings_get();
    if (settings != NULL && simple_menu_layer != NULL) {
        menu_layer_set_highlight_colors(
            simple_menu_layer_get_menu_layer(simple_menu_layer),
            settings->accent_color,
            GColorWhite
        );
    }
}

void main_menu_deinit() {
    if (simple_menu_layer != NULL) {
        simple_menu_layer_destroy(simple_menu_layer);
    }

    if (window != NULL) {
        window_destroy(window);
    }
}

void main_menu_mark_members_loaded() {
    items[0].subtitle = "members loaded!";
    layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
    members_loaded = true;
}
