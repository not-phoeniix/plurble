#include "fronters_menu.h"
#include <pebble.h>
#include "member_menu.h"
#include "../members/member_collections.h"

static MemberMenu* menu = NULL;

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    member_menu_draw_cell(menu, ctx, cell_layer, cell_index);
}

static void select(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    member_menu_select_member(menu, cell_index);
}

void fronters_menu_push() {
    if (menu == NULL) {
        MemberMenuCallbacks callbacks = {
            .draw_row = draw_row,
            .select = select,
            .window_load = NULL,
            .window_unload = NULL
        };

        menu = member_menu_create(callbacks, members_get_fronters());
    }

    member_menu_window_push(menu);
}

void fronters_menu_deinit() {
    if (menu != NULL) {
        member_menu_destroy(menu);
        menu = NULL;
    }
}

void fronters_menu_update_colors() {
    if (menu != NULL) {
        member_menu_update_colors(menu);
    }
}
