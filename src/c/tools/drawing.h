#pragma once

#include <pebble.h>
#include "../member.h"

GColor drawing_get_text_color(GColor background);
void drawing_draw_member_cell(GContext* ctx, Member* member, MenuLayer* menu_layer, const Layer* cell_layer, GColor highlight_color);
void drawing_deinit();
