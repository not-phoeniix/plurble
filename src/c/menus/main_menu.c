#include "main_menu.h"
#include "members_menu.h"
#include "../tools/string_tools.h"
#include "../config/config.h"

static Window* window = NULL;
static SimpleMenuLayer* simple_menu_layer = NULL;
static SimpleMenuItem items[1];
static SimpleMenuSection sections[1];

static void select(int index, void* context) {
    switch (index) {
        // "members" case
        case 0:
            // members_menu_push();
            break;
    }

    members_menu_push();
}

static void window_load() {
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    items[0] = (SimpleMenuItem) {
        .title = "Members",
        .subtitle = "No one fronting :O",
        .icon = NULL,
        .callback = select
    };

    sections[0] = (SimpleMenuSection) {
        .items = items,
        .num_items = 1,
        .title = NULL
    };

    simple_menu_layer = simple_menu_layer_create(bounds, window, sections, 1, NULL);

    ClaySettings* settings = settings_get();
    menu_layer_set_highlight_colors(
        simple_menu_layer_get_menu_layer(simple_menu_layer),
        settings->accent_color,
        GColorWhite
    );

    layer_add_child(window_layer, simple_menu_layer_get_layer(simple_menu_layer));
}

static void window_unload() {
    simple_menu_layer_destroy(simple_menu_layer);
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
