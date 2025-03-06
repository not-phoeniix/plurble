#include "config.h"
#include "../messaging/messaging.h"
#include "../menus/main_menu.h"
#include "../menus/all_members_menu.h"
#include "../menus/custom_fronts_menu.h"
#include "../menus/fronters_menu.h"
#include "../members/member_collections.h"

#define SETTINGS_KEY 1

static ClaySettings settings;

static void set_defaults() {
    settings.accent_color = GColorChromeYellow;
    settings.background_color = GColorWhite;
    settings.compact_member_list = false;
    settings.member_color_highlight = false;
    settings.member_color_tag = PBL_IF_COLOR_ELSE(true, false);
    settings.global_fronter_accent = false;
}

static void apply() {
    main_menu_update_colors();
    all_members_menu_update_colors();
    custom_fronts_menu_update_colors();
    fronters_menu_update_colors();

    // mark current window layer for redraw on settings application
    Window* top_window = window_stack_get_top_window();
    if (top_window != NULL) {
        Layer* window_layer = window_get_root_layer(top_window);
        layer_mark_dirty(window_layer);
    }
}

GColor settings_get_global_accent() {
    Member* first_fronter = members_get_first_fronter();

    if (settings.global_fronter_accent && first_fronter != NULL) {
        return first_fronter->color;
    } else {
        return settings.accent_color;
    }
}

ClaySettings* settings_get() {
    return &settings;
}

void settings_load() {
    set_defaults();
    persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
    apply();
}

void settings_save() {
    persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
    apply();
}
