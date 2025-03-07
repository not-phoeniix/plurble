#pragma once

#include <pebble.h>
#include "../config/config.h"
#include "../members/member.h"

struct MemberMenu;
typedef struct MemberMenu MemberMenu;

typedef struct {
    MenuLayerSelectCallback select;
    MenuLayerDrawRowCallback draw_row;
    WindowHandler window_load;
    WindowHandler window_unload;
} MemberMenuCallbacks;

void member_menu_draw_cell(MemberMenu* menu, GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index);
void member_menu_select_member(MemberMenu* menu, MenuIndex* cell_index);
void member_menu_update_colors(MemberMenu* menu);
MemberMenu* member_menu_create(MemberMenuCallbacks callbacks, MemberList* members);
void member_menu_destroy(MemberMenu* menu);
void member_menu_window_push(MemberMenu* menu);
void member_menu_window_remove(MemberMenu* menu);
void member_menu_set_members(MemberMenu* menu, MemberList* members);
MemberList* member_menu_get_members(MemberMenu* menu);
Window* member_menu_get_window(MemberMenu* menu);
