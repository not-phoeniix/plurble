#pragma once

#include <pebble.h>
#include "../data/config.h"
#include "../frontables/frontable_list.h"
#include "../frontables/group.h"

struct FrontableMenu;
typedef struct FrontableMenu FrontableMenu;

typedef struct {
    MenuLayerSelectCallback select;
    MenuLayerDrawRowCallback draw_row;
    WindowHandler window_load;
    WindowHandler window_unload;
} MemberMenuCallbacks;

void frontable_menu_draw_cell(FrontableMenu* menu, GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index);
void frontable_menu_select(FrontableMenu* menu, MenuIndex* cell_index);
void frontable_menu_update_colors(FrontableMenu* menu);
FrontableMenu* frontable_menu_create(MemberMenuCallbacks callbacks, FrontableList* frontables, Group* groups, uint16_t num_groups, const char* name);
void frontable_menu_destroy(FrontableMenu* menu);
void frontable_menu_window_push(FrontableMenu* menu);
void frontable_menu_window_remove(FrontableMenu* menu);
void frontable_menu_set_frontables(FrontableMenu* menu, FrontableList* frontables);
FrontableList* frontable_menu_get_frontables(FrontableMenu* menu);
Window* frontable_menu_get_window(FrontableMenu* menu);
