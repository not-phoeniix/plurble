#pragma once

#include <pebble.h>

typedef struct {
    GColor accent_color;
    bool compact_member_list;
    bool member_color_highlight;
} ClaySettings;

ClaySettings* settings_get();

void settings_load();
void settings_save();
