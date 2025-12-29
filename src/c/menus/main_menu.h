#pragma once

#include <pebble.h>

void main_menu_push();
void main_menu_update_colors();
void main_menu_deinit();
void main_menu_mark_members_loaded();
void main_menu_mark_custom_fronts_loaded();
void main_menu_mark_fronters_loaded();
void main_menu_update_fronters_subtitle();
void main_menu_update_fetch_status(bool fetching);
