#include "config.h"
#include "../messaging/messaging.h"
#include "../menus/main_menu.h"
#include "../menus/members_menu.h"

#define SETTINGS_KEY 1

static ClaySettings settings;

static void set_defaults() {
    settings.accent_color = GColorRed;
}

static void apply() {
    main_menu_update_colors();
    members_menu_update_colors();

    // mark current window layer for redraw on settings application
    Window* top_window = window_stack_get_top_window();
    if (top_window != NULL) {
        Layer* window_layer = window_get_root_layer(top_window);
        layer_mark_dirty(window_layer);
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
