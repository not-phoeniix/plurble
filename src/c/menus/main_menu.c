#include "main_menu.h"
#include "../data/config.h"
#include "../data/frontable_cache.h"
#include "../frontables/frontable_list.h"
#include "members_menu.h"
#include "current_fronters_menu.h"
#include "custom_fronts_menu.h"
#include "../messaging/messaging.h"
#include "../tools/string_tools.h"
#include "settings_menu.h"

static Window* window = NULL;
static SimpleMenuLayer* simple_menu_layer = NULL;
static SimpleMenuItem items[4];
static SimpleMenuSection sections[1];
static TextLayer* status_bar_text = NULL;
static Layer* status_bar_layer = NULL;
static bool members_loaded = false;
static bool custom_fronts_loaded = false;
static bool current_fronters_loaded = false;

static void select(int index, void* context) {
    switch (index) {
        case 0:
            if (current_fronters_loaded) {
                current_fronters_menu_push();
            }
            break;
        case 1:
            if (members_loaded) {
                members_menu_push();
            }
            break;
        case 2:
            if (custom_fronts_loaded) {
                custom_fronts_menu_push();
            }
            break;
        case 3:
            settings_menu_push();
            break;
    }
}

static void status_bar_update_proc(Layer* layer, GContext* ctx) {
    graphics_context_set_fill_color(ctx, settings_get()->background_color);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void window_load() {
    Layer* root_layer = window_get_root_layer(window);

    // ~~~ create menu items ~~~

    items[0] = (SimpleMenuItem) {
        .title = "Fronters",
        .subtitle = current_fronters_loaded ? "no one is fronting" : "loading fronters...",
        .icon = NULL,
        .callback = select
    };

    items[1] = (SimpleMenuItem) {
        .title = "Members",
        .subtitle = members_loaded ? NULL : "loading members...",
        .icon = NULL,
        .callback = select
    };

    items[2] = (SimpleMenuItem) {
        .title = "Custom Fronts",
        .subtitle = custom_fronts_loaded ? NULL : "loading custom fronts...",
        .icon = NULL,
        .callback = select
    };

    items[3] = (SimpleMenuItem) {
        .title = "Settings",
        .subtitle = NULL,
        .icon = NULL,
        .callback = select
    };

    sections[0] = (SimpleMenuSection) {
        .items = items,
        .num_items = 4,
    };

    GRect menu_bounds = layer_get_bounds(root_layer);

#if !defined(PBL_ROUND)
    // only offset if not round, that way round watches
    //   keep the highlighted option centered !
    menu_bounds.origin.y += STATUS_BAR_LAYER_HEIGHT;
    menu_bounds.size.h -= STATUS_BAR_LAYER_HEIGHT;
#endif

    simple_menu_layer = simple_menu_layer_create(
        menu_bounds,
        window,
        sections,
        1,
        NULL
    );

    layer_add_child(root_layer, simple_menu_layer_get_layer(simple_menu_layer));
    main_menu_update_colors();

    // ~~~ create status bar layers ~~~

    GRect status_bar_bounds = layer_get_bounds(root_layer);
    status_bar_bounds.size.h = STATUS_BAR_LAYER_HEIGHT;

    GRect status_bar_text_bounds = status_bar_bounds;
    status_bar_text_bounds.size.h = 14;
    grect_align(&status_bar_text_bounds, &status_bar_bounds, GAlignCenter, false);

#if !defined(PBL_ROUND)
    // shift status bar text up 3 pixels for non round watches!
    status_bar_text_bounds.origin.y -= 3;
#endif

    status_bar_layer = layer_create(status_bar_bounds);
    layer_set_update_proc(status_bar_layer, status_bar_update_proc);

    status_bar_text = text_layer_create(status_bar_text_bounds);
    text_layer_set_font(status_bar_text, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(status_bar_text, GTextAlignmentCenter);
    text_layer_set_text(status_bar_text, "Plurble");

    layer_add_child(status_bar_layer, text_layer_get_layer(status_bar_text));
    layer_add_child(root_layer, status_bar_layer);
}

static void window_unload() {
    simple_menu_layer_destroy(simple_menu_layer);
    simple_menu_layer = NULL;
    text_layer_destroy(status_bar_text);
    status_bar_text = NULL;
    layer_destroy(status_bar_layer);
    status_bar_layer = NULL;
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

    if (status_bar_text != NULL) {
        text_layer_set_background_color(status_bar_text, settings->background_color);
        text_layer_set_text_color(status_bar_text, gcolor_legible_over(settings->background_color));
    }

    if (window != NULL) {
        window_set_background_color(window, settings->background_color);
    }
}

void main_menu_deinit() {
    if (window != NULL) {
        window_destroy(window);
        window = NULL;
    }
}

void main_menu_mark_fronters_loaded() {
    items[0].subtitle = NULL;
    if (simple_menu_layer != NULL) {
        layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
    }

    current_fronters_loaded = true;
}

void main_menu_mark_members_loaded() {
    items[1].subtitle = NULL;
    if (simple_menu_layer != NULL) {
        layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
    }

    members_loaded = true;
}

void main_menu_mark_custom_fronts_loaded() {
    items[2].subtitle = NULL;
    if (simple_menu_layer != NULL) {
        layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
    }

    custom_fronts_loaded = true;
}

void main_menu_set_members_subtitle(const char* subtitle) {
    static char s_subtitle[33];
    strncpy(s_subtitle, subtitle, sizeof(s_subtitle));
    items[1].subtitle = s_subtitle;
}

void main_menu_set_custom_fronts_subtitle(const char* subtitle) {
    static char s_subtitle[33];
    strncpy(s_subtitle, subtitle, sizeof(s_subtitle));
    items[2].subtitle = s_subtitle;
}

void main_menu_set_fronters_subtitle(const char* subtitle) {
    static char s_subtitle[33];
    strncpy(s_subtitle, subtitle, sizeof(s_subtitle));
    items[0].subtitle = s_subtitle;
}
