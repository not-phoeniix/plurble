#pragma once

#include <pebble.h>
#include "../data/config.h"
#include "../frontables/frontable_list.h"
#include "../frontables/group.h"

struct FrontableMenu;
typedef struct FrontableMenu FrontableMenu;

typedef void (*FrontableMenuDrawRowCallback)(
    FrontableMenu* menu,
    GContext* ctx,
    const Layer* cell_layer,
    Frontable* selected_frontable,
    Group* selected_group
);

typedef struct {
    MenuLayerSelectCallback select;
    FrontableMenuDrawRowCallback draw_row;
    WindowHandler window_load;
    WindowHandler window_unload;
} MemberMenuCallbacks;

void frontable_menu_draw_cell_custom(FrontableMenu* menu, GContext* ctx, const Layer* cell_layer, const char* main_text, const char* bottom_left_text, const char* bottom_right_text, GColor tag_color);
void frontable_menu_draw_cell_default(FrontableMenu* menu, GContext* ctx, const Layer* cell_layer, Frontable* selected_frontable, Group* selected_group);
void frontable_menu_select(FrontableMenu* menu, MenuIndex* cell_index);
void frontable_menu_update_colors(FrontableMenu* menu);
FrontableMenu* frontable_menu_create(MemberMenuCallbacks callbacks, Group* group);
void frontable_menu_destroy(FrontableMenu* menu);
void frontable_menu_window_push(FrontableMenu* menu, bool recursive, bool animated);
void frontable_menu_window_remove(FrontableMenu* menu);
void frontable_menu_window_pop_to_root(FrontableMenu* menu, bool animated);
void frontable_menu_set_frontables(FrontableMenu* menu, FrontableList* frontables);
void frontable_menu_set_parent(FrontableMenu* menu, FrontableMenu* parent);
void frontable_menu_clear_children(FrontableMenu* menu);
FrontableList* frontable_menu_get_frontables(FrontableMenu* menu);
Window* frontable_menu_get_window(FrontableMenu* menu);
MenuIndex frontable_menu_get_selected_index(FrontableMenu* menu);
MenuLayer* frontable_menu_get_menu_layer(FrontableMenu* menu);
GColor frontable_menu_get_current_highlight_color(FrontableMenu* menu);
void frontable_menu_set_selected_index(FrontableMenu* menu, uint16_t index);
void frontable_menu_clamp_selected_index(FrontableMenu* menu);
const char* frontable_menu_get_name(FrontableMenu* menu);
