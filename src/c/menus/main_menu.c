#include "main_menu.h"
#include "../data/config.h"
#include "../data/frontable_cache.h"
#include "../frontables/frontable_list.h"
#include "members_menu.h"
#include "current_fronters_menu.h"
#include "custom_fronts_menu.h"
#include "../messaging/messaging.h"
#include "../tools/string_tools.h"

static Window* window = NULL;
static SimpleMenuLayer* simple_menu_layer = NULL;
static SimpleMenuItem member_items[3];
static SimpleMenuItem extra_items[2];
static SimpleMenuItem config_items[2];
static SimpleMenuSection sections[2];
static TextLayer* status_bar_text = NULL;
static Layer* status_bar_layer = NULL;
static bool members_loaded = false;
static bool custom_fronts_loaded = false;
static bool current_fronters_loaded = false;
static bool can_fetch_members = true;
static bool confirm_clear_cache = false;
static AppTimer* confirm_clear_cache_timer = NULL;

static void member_select(int index, void* context) {
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
    }
}

static void extra_select(int index, void* context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "polls !!");
}

static void reset_fetch_name(void* data) {
    config_items[0].subtitle = "Re-fetch frontables...";
    can_fetch_members = true;

    layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
}

static void reset_cache_confirm(void* data) {
    confirm_clear_cache = false;
    config_items[1].subtitle = "Will reset app...";
    config_items[1].title = "Clear Cache";

    if (confirm_clear_cache_timer != NULL) {
        app_timer_cancel(confirm_clear_cache_timer);
        confirm_clear_cache_timer = NULL;
    }

    layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
}

static void config_select(int index, void* context) {
    switch (index) {
        case 0:
            if (can_fetch_members) {
                messaging_fetch_fronters();
                config_items[0].subtitle = "Fetched :D";
                can_fetch_members = false;
                app_timer_register(4000, reset_fetch_name, NULL);
            }
            break;
        case 1:
            if (!confirm_clear_cache) {
                config_items[1].subtitle = "Click to confirm";
                config_items[1].title = "U SURE?";
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

static void status_bar_update_proc(Layer* layer, GContext* ctx) {
    graphics_context_set_fill_color(ctx, settings_get()->background_color);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void window_load() {
    Layer* root_layer = window_get_root_layer(window);

    // ~~~ create menu items ~~~

    member_items[0] = (SimpleMenuItem) {
        .title = "Fronters",
        .subtitle = current_fronters_loaded ? "no one is fronting" : "loading fronters...",
        .icon = NULL,
        .callback = member_select
    };

    member_items[1] = (SimpleMenuItem) {
        .title = "Members",
        .subtitle = members_loaded ? NULL : "loading members...",
        .icon = NULL,
        .callback = member_select
    };

    member_items[2] = (SimpleMenuItem) {
        .title = "Custom Fronts",
        .subtitle = custom_fronts_loaded ? NULL : "loading custom fronts...",
        .icon = NULL,
        .callback = member_select
    };

    sections[0] = (SimpleMenuSection) {
        .items = member_items,
        .num_items = 3,
        .title = "Member Management"
    };

    // extra_items[0] = (SimpleMenuItem) {
    //     .title = "Polls",
    //     .subtitle = NULL,
    //     .icon = NULL,
    //     .callback = extra_select
    // };
    //
    // extra_items[1] = (SimpleMenuItem) {
    //     .title = "Chat",
    //     .subtitle = NULL,
    //     .icon = NULL,
    //     .callback = extra_select
    // };
    //
    // sections[1] = (SimpleMenuSection) {
    //     .items = extra_items,
    //     .num_items = 2,
    //     .title = "Extra"
    // };

    config_items[0] = (SimpleMenuItem) {
        .title = "Refresh Data",
        .subtitle = "Re-fetch frontables...",
        .icon = NULL,
        .callback = config_select,
    };

    config_items[1] = (SimpleMenuItem) {
        .title = "Clear Cache",
        .subtitle = "Will reset app...",
        .icon = NULL,
        .callback = config_select,
    };

    sections[1] = (SimpleMenuSection) {
        .items = config_items,
        .num_items = 2,
        .title = "Config"
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
        2,
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
    }
}

void main_menu_mark_fronters_loaded() {
    member_items[0].subtitle = NULL;
    if (simple_menu_layer != NULL) {
        layer_mark_dirty(simple_menu_layer_get_layer(simple_menu_layer));
    }

    current_fronters_loaded = true;
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

void main_menu_set_members_subtitle(const char* subtitle) {
    static char s_subtitle[33];
    string_copy_smaller(s_subtitle, subtitle, 32);
    member_items[1].subtitle = s_subtitle;
}

void main_menu_set_custom_fronts_subtitle(const char* subtitle) {
    static char s_subtitle[33];
    string_copy_smaller(s_subtitle, subtitle, 32);
    member_items[2].subtitle = s_subtitle;
}

void main_menu_set_fronters_subtitle(const char* subtitle) {
    static char s_subtitle[33];
    string_copy_smaller(s_subtitle, subtitle, 32);
    member_items[0].subtitle = s_subtitle;
}
