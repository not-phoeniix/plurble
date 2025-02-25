#pragma once

#include <pebble.h>

typedef struct {
    GColor accent_color;
} ClaySettings;

ClaySettings* settings_get();

void settings_load();
void settings_save();
