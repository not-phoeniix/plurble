#include "main_menu.h"
#include "../tools/string_tools.h"
#include "../config/config.h"
#include "../members/member_collections.h"
#include "all_members_menu.h"
#include "fronters_menu.h"
#include "custom_fronts_menu.h"

static Window* window = NULL;
static SimpleMenuLayer* simple_menu_layer = NULL;
static SimpleMenuItem member_items[3];
static SimpleMenuItem extra_items[1];
static SimpleMenuSection sections[2];
static bool members_loaded = false;
static bool custom_fronts_loaded = false;

static void member_select(int index, void* context) {
    switch (index) {
        case 0:
            if (members_loaded && custom_fronts_loaded) {
                fronters_menu_push();
            }
            break;
        case 1:
            if (members_loaded) {
                all_members_menu_push();
            }
            break;
        case 2:
            if (custom_fronts_loaded) {
                custom_fronts_menu_push();
            }
            break;
    }
}

static void extra_select(int index, void* context) {
    printf("polls! ");
}

static void window_load() {
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    bool fronters_loaded = members_loaded && custom_fronts_loaded;
    member_items[0] = (SimpleMenuItem) {
        .title = "Fronters",
        .subtitle = fronters_loaded ? "no one is fronting" : "loading fronters...",
        .icon = NULL,
        .callback = member_select
    };

    member_items[1] = (SimpleMenuItem) {
        .title = "Member List",
        .subtitle = members_loaded ? NULL : "loading members...",
        .icon = NULL,
        .callback = member_select
    };

    member_items[2] = (SimpleMenuItem) {
        .title = "Custom Front",
        .subtitle = custom_fronts_loaded ? NULL : "loading custom fronts...",
        .icon = NULL,
        .callback = member_select
    };

    sections[0] = (SimpleMenuSection) {
        .items = member_items,
        .num_items = 3,
        .title = "Member Management"
    };

    extra_items[0] = (SimpleMenuItem) {
        .title = "Polls",
        .subtitle = NULL,
        .icon = NULL,
        .callback = extra_select
    };

    sections[1] = (SimpleMenuSection) {
        .items = extra_items,
        .num_items = 1,
        .title = "Extra"
    };

    simple_menu_layer = simple_menu_layer_create(bounds, window, sections, 2, NULL);

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
    if (settings != NULL) {
        if (simple_menu_layer != NULL) {
            menu_layer_set_highlight_colors(
                simple_menu_layer_get_menu_layer(simple_menu_layer),
                settings_get_global_accent(),
                gcolor_legible_over(settings_get_global_accent())
            );
            menu_layer_set_normal_colors(
                simple_menu_layer_get_menu_layer(simple_menu_layer),
                settings->background_color,
                gcolor_legible_over(settings->background_color)
            );
        }

        if (window != NULL) {
            window_set_background_color(window, settings->background_color);
        }
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
    member_items[1].subtitle = NULL;
    if (simple_menu_layer != NULL) {
        layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
    }

    members_loaded = true;
}

void main_menu_mark_custom_fronts_loaded() {
    member_items[2].subtitle = NULL;
    if (simple_menu_layer != NULL) {
        layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
    }

    custom_fronts_loaded = true;
}

void main_menu_set_fronters_subtitle(const char* subtitle) {
    member_items[0].subtitle = subtitle;
}
