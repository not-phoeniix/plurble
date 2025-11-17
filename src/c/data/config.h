#pragma once

#include <pebble.h>
#include "../frontables/frontable.h"

typedef struct {
    GColor accent_color;
    GColor background_color;
    bool compact_member_list;
    bool member_color_tag;
    bool member_color_highlight;
    bool global_fronter_accent;
    bool group_title_accent;
    bool api_key_valid;
} ClaySettings;

ClaySettings* settings_get();
GColor settings_get_global_accent();

void settings_load();
void settings_save(bool update_colors);
