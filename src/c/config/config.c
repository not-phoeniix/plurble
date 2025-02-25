#include "config.h"
#include "../menus/menus.h"

#define SETTINGS_KEY 1

static ClaySettings settings;

static void set_defaults() {
    settings.accent_color = GColorRed;
    settings.plural_api_key = "";
}

static void apply() {
    // menu_set_highlight_colors(settings.accent_color, GColorWhite);
    // menu_mark_dirty();
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
