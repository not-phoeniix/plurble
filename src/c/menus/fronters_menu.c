#include "fronters_menu.h"
#include "member_menu.h"
#include "../data/frontable_cache.h"

static MemberMenu* menu = NULL;
static TextLayer* text_layer = NULL;
static bool empty = false;

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    member_menu_draw_cell(menu, ctx, cell_layer, cell_index);
}

static void select(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    member_menu_select_member(menu, cell_index);
}

static void window_load(Window* window) {
    Layer* root_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root_layer);
    GRect text_bounds = {
        .origin = (GPoint) {.x = 0, .y = 0},
        .size = {
            .w = 130,
            .h = 80
        }
    };

    grect_align(&text_bounds, &bounds, GAlignCenter, false);

    text_layer = text_layer_create(text_bounds);
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text(text_layer, "No one is currently fronting...");
    text_layer_set_background_color(text_layer, settings_get()->background_color);
    text_layer_set_text_color(text_layer, gcolor_legible_over(settings_get()->background_color));
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);

    if (cache_get_current_fronters()->num_stored <= 0) {
        layer_add_child(root_layer, text_layer_get_layer(text_layer));
        empty = true;
    } else {
        empty = false;
    }
}

static void window_unload(Window* window) {
    text_layer_destroy(text_layer);
    text_layer = NULL;
}

void fronters_menu_push() {
    if (menu == NULL) {
        MemberMenuCallbacks callbacks = {
            .draw_row = draw_row,
            .select = select,
            .window_load = window_load,
            .window_unload = window_unload
        };

        menu = member_menu_create(callbacks, cache_get_current_fronters(), "Fronters");
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

    if (text_layer != NULL) {
        text_layer_set_background_color(text_layer, settings_get()->background_color);
        text_layer_set_text_color(text_layer, gcolor_legible_over(settings_get()->background_color));
    }
}

void fronters_menu_set_is_empty(bool p_empty) {
    // don't update anything if text layer doesn't exist lol
    if (text_layer == NULL) {
        return;
    }

    // don't update any states if the empty flag is
    //   already the same as the inputted state
    if (empty == p_empty) {
        return;
    }

    // get layer pointers
    Window* window = member_menu_get_window(menu);
    Layer* root_layer = window_get_root_layer(window);
    Layer* layer = text_layer_get_layer(text_layer);

    // always remove parent so duplicates do not occur, then
    //   if set to show then re-add layer as child of root
    if (p_empty) {
        layer_add_child(root_layer, layer);
    } else {
        layer_remove_from_parent(layer);
    }

    // mark to redraw every update, update boolean flag
    empty = p_empty;
    layer_mark_dirty(root_layer);
}
