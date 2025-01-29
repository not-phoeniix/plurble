#pragma once

#include <pebble.h>

typedef struct {
    char* plural_api_key;
    GColor accent_color;
} ClaySettings;

ClaySettings* settings_get();

void settings_load();
void settings_save();
