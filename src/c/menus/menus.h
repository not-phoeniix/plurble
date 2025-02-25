#pragma once

#include <pebble.h>

int16_t add_menu(
    MenuLayerSelectCallback click_callback,
    void (*unload_callback)(),
    char** options,
    uint16_t num_options
);
Window* get_menu(int16_t index);
bool menu_is_loaded(int16_t index);
void menu_window_push(int16_t index);
void menu_set_highlight_colors(GColor background, GColor foreground);
void menu_deinit();
void menu_mark_dirty();
