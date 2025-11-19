#include "config.h"
#include "../messaging/messaging.h"
#include "../menus/main_menu.h"
#include "../menus/members_menu.h"
#include "../menus/custom_fronts_menu.h"
#include "../menus/current_fronters_menu.h"
#include "../data/frontable_cache.h"
#include "../menus/setup_prompt_menu.h"

#define SETTINGS_KEY 1

static ClaySettings settings;

static void set_defaults() {
    settings.accent_color = GColorChromeYellow;
    settings.background_color = GColorWhite;
    settings.compact_member_list = false;
    settings.member_color_highlight = false;
    settings.member_color_tag = PBL_IF_COLOR_ELSE(PBL_IF_ROUND_ELSE(false, true), false);
    settings.global_fronter_accent = false;
    settings.api_key_valid = false;
    settings.group_title_accent = false;
}

static void apply(bool update_colors) {
    if (update_colors) {
        main_menu_update_colors();
        members_menu_update_colors();
        custom_fronts_menu_update_colors();
        current_fronters_menu_update_colors();
    }

    if (settings.api_key_valid && setup_prompt_menu_shown()) {
        window_stack_pop_all(false);
        main_menu_push();
    } else if (!settings.api_key_valid) {
        window_stack_pop_all(false);
        setup_prompt_menu_push();
    }

    // always redraw currently shown window when applying config
    Window* top_window = window_stack_get_top_window();
    if (top_window != NULL) {
        Layer* window_layer = window_get_root_layer(top_window);
        layer_mark_dirty(window_layer);
    }
}

GColor settings_get_global_accent() {
    Frontable* first_fronter = cache_get_first_fronter();

    GColor color;

    if (settings.global_fronter_accent && first_fronter != NULL) {
        color = frontable_get_color(first_fronter);
    } else {
        color = settings.accent_color;
    }

    if (color.argb == settings.background_color.argb) {
        color = gcolor_legible_over(settings.background_color);
    }

    return color;
}

ClaySettings* settings_get() {
    return &settings;
}

void settings_load() {
    set_defaults();
    persist_read_data(SETTINGS_KEY, &settings, sizeof(ClaySettings));
    apply(true);
}

void settings_save(bool update_colors) {
    persist_write_data(SETTINGS_KEY, &settings, sizeof(ClaySettings));
    apply(update_colors);
}
