#pragma once

#include <pebble.h>

void members_menu_push();
void members_set_members(char* members, const char delim);
void members_menu_update_colors();
void members_menu_deinit();
