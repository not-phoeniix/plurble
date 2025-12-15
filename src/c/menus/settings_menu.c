#include "settings_menu.h"
#include <pebble.h>
#include "../data/config.h"
#include "../messaging/messaging.h"
#include "../data/frontable_cache.h"
#include "../menus/members_menu.h"

static Window* window = NULL;
static SimpleMenuLayer* simple_menu_layer = NULL;
static SimpleMenuItem items[3];
static SimpleMenuSection sections[1];
static TextLayer* status_bar_text = NULL;
static Layer* status_bar_layer = NULL;
static bool can_fetch_members = true;
static bool confirm_clear_cache = false;
static AppTimer* confirm_clear_cache_timer = NULL;
static AppTimer* fetch_timeout_timer = NULL;

static void status_bar_update_proc(Layer* layer, GContext* ctx) {
    graphics_context_set_fill_color(ctx, settings_get()->background_color);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void reset_fetch_name_callback(void* data) {
    items[1].subtitle = "Re-fetch from API...";
    can_fetch_members = true;

    if (simple_menu_layer != NULL) {
        layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
    }
}

static void fetch_timeout_name_callback(void* data) {
    items[1].subtitle = "Fetch timed out :(";
    if (simple_menu_layer != NULL) {
        layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
    }

    fetch_timeout_timer = NULL;

    app_timer_register(4000, reset_fetch_name_callback, NULL);
}

void settings_menu_confirm_frontable_fetch() {
    if (fetch_timeout_timer != NULL) {
        app_timer_cancel(fetch_timeout_timer);
        fetch_timeout_timer = NULL;
    }

    items[1].subtitle = "Fetched :D";
    app_timer_register(2000, reset_fetch_name_callback, NULL);
}

static void reset_cache_confirm(void* data) {
    confirm_clear_cache = false;
    items[2].subtitle = "Will reset app...";
    items[2].title = "Clear Cache";

    if (confirm_clear_cache_timer != NULL) {
        app_timer_cancel(confirm_clear_cache_timer);
        confirm_clear_cache_timer = NULL;
    }

    if (simple_menu_layer != NULL) {
        layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
    }
}

static void select(int index, void* context) {
    switch (index) {
        case 0:
            settings_get()->show_groups = !settings_get()->show_groups;

            items[0].subtitle = settings_get()->show_groups ? "True" : "False";
            if (simple_menu_layer != NULL) {
                layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
            }

            members_menu_remove_groups();
            members_menu_create_groups();
            members_menu_refresh_groupless_members();
            break;

        case 1:
            if (can_fetch_members) {
                messaging_fetch_data();
                items[1].subtitle = "Fetching...";
                can_fetch_members = false;
                fetch_timeout_timer = app_timer_register(10000, fetch_timeout_name_callback, NULL);
            }
            break;

        case 2:
            if (!confirm_clear_cache) {
                items[2].subtitle = "Click to confirm";
                items[2].title = "U SURE?";
                confirm_clear_cache = true;
                app_timer_register(2000, reset_cache_confirm, NULL);
            } else {
                reset_cache_confirm(NULL);
                cache_persist_delete();
                messaging_clear_cache();
            }
            break;
    }

    layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
}

static void window_load() {
    Layer* root_layer = window_get_root_layer(window);

    items[0] = (SimpleMenuItem) {
        .title = "Show Groups",
        .subtitle = settings_get()->show_groups ? "True" : "False",
        .icon = NULL,
        .callback = select
    };

    items[1] = (SimpleMenuItem) {
        .title = "Refresh Data",
        .subtitle = "Re-fetch from API...",
        .icon = NULL,
        .callback = select,
    };

    items[2] = (SimpleMenuItem) {
        .title = "Clear Cache",
        .subtitle = "Will reset app...",
        .icon = NULL,
        .callback = select,
    };

    sections[0] = (SimpleMenuSection) {
        .num_items = 3,
        .items = items
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
    settings_menu_update_colors();

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
    text_layer_set_text(status_bar_text, "Settings");

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

void settings_menu_push() {
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

void settings_menu_update_colors() {
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

void settings_menu_deinit() {
    if (window != NULL) {
        window_destroy(window);
        window = NULL;
    }
}
