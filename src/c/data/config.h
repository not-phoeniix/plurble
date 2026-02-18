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
    bool show_groups;
    bool hide_members_in_root;
    bool show_pronouns;
    bool show_time_fronting;
    char custom_front_text[FRONTABLE_PRONOUNS_LENGTH];
} ClaySettings;

ClaySettings* settings_get();
GColor settings_get_global_accent();

void settings_load();
void settings_save(bool update_colors);
