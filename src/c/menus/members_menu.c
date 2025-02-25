#include "members_menu.h"
#include "../tools/string_tools.h"
#include "../config/config.h"

static Window* window = NULL;
static MenuLayer* menu_layer = NULL;
static char** members = NULL;
static uint16_t num_members = 0;

static void select(
    MenuLayer* menu_layer,
    MenuIndex* menu_index,
    void* context
) {
    char* member = members[menu_index->row];
    printf("wow! you clicked... [%s]!!", member);
}

static uint16_t get_num_rows(MenuLayer* layer, uint16_t selected_index, void* ctx) {
    return num_members;
}

static int16_t get_cell_height(struct MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    return 44;
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    menu_cell_basic_draw(ctx, cell_layer, members[cell_index->row], NULL, NULL);
}

static void window_load() {
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(
        menu_layer,
        NULL,
        (MenuLayerCallbacks) {
            .get_num_rows = get_num_rows,
            .draw_row = draw_row,
            .get_cell_height = get_cell_height,
            .select_click = select
        }
    );

    members_menu_update_colors();

    menu_layer_set_click_config_onto_window(menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

static void window_unload() {
    menu_layer_destroy(menu_layer);
    menu_layer = NULL;
}

void members_menu_push() {
    if (window == NULL) {
        window = window_create();
        window_set_window_handlers(
            window,
            (WindowHandlers) {
                .load = window_load,
                .unload = window_unload
            }
        );
    }

    window_stack_push(window, true);
}

void members_set_members(char* p_members, char delim) {
    if (members != NULL) {
        string_array_free(members, num_members);
    }

    members = string_split(p_members, delim, &num_members);
}

void members_menu_update_colors() {
    ClaySettings* settings = settings_get();
    if (settings != NULL && menu_layer != NULL) {
        menu_layer_set_highlight_colors(
            menu_layer,
            settings->accent_color,
            GColorWhite
        );
    }
}

void members_menu_deinit() {
    if (members != NULL) {
        string_array_free(members, num_members);
        members = NULL;
    }

    if (menu_layer != NULL) {
        menu_layer_destroy(menu_layer);
    }

    if (window != NULL) {
        window_destroy(window);
    }
}
