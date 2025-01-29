#include "menus.h"

#include <pebble.h>
#include <stdlib.h>
#include "../config/config.h"
#include "../tools/string_tools.h"

#define MAX_MENUS 10

typedef struct {
    Window* window;
    MenuLayer* menu_layer;
    char** options;
    uint16_t num_options;
    MenuLayerSelectCallback click_callback;
    void (*unload_callback)();
} MenuData;

static int16_t num_windows = 0;
static MenuData menus[MAX_MENUS] = {{
    .window = NULL,
    .menu_layer = NULL,
    .options = NULL,
    .num_options = 0,
    .click_callback = NULL,
    .unload_callback = NULL
}};

static int16_t get_window_index(Window* window) {
    for (int i = 0; i < MAX_MENUS; i++) {
        if (menus[i].window == window) {
            return i;
        }
    }

    return -1;
}

static uint16_t get_num_rows(
    MenuLayer* menu_layer,
    uint16_t selected_index,
    void* context
) {
    // cast context to window index and return number at that index
    Window* window = layer_get_window(menu_layer_get_layer(menu_layer));
    int16_t window_index = get_window_index(window);
    return menus[window_index].num_options;
}

static void draw_row(
    GContext* ctx,
    const Layer* cell_layer,
    MenuIndex* cell_index,
    void* context
) {
    // cast context to window index and grab data there
    Window* window = layer_get_window(cell_layer);
    int16_t window_index = get_window_index(window);
    char** options = menus[window_index].options;

    // draw cel with text of options at given cell index
    menu_cell_basic_draw(ctx, cell_layer, options[cell_index->row], NULL, NULL);
}

static void window_load(Window* window) {
    int16_t window_index = get_window_index(window);
    if (window_index == -1) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Window load error: window unable to be found in array!");
        return;
    }

    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    MenuLayerSelectCallback select_callback = menus[window_index].click_callback;

    // create menu layer and set up callback functions
    MenuLayer* menu_layer = menu_layer_create(bounds);
    menu_layer_set_click_config_onto_window(menu_layer, window);
    menu_layer_set_callbacks(
        menu_layer,
        NULL,
        (MenuLayerCallbacks) {
            .get_num_rows = get_num_rows,
            .select_click = select_callback,
            .draw_row = draw_row
        }
    );

    ClaySettings* settings = settings_get();
    menu_layer_set_highlight_colors(menu_layer, settings->accent_color, GColorWhite);

    menus[window_index].menu_layer = menu_layer;

    layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

static void window_unload(Window* window) {
    int16_t index = get_window_index(window);
    if (index == -1) {
        printf("trying to unload a window that doesn't exist...?");
        return;
    }

    // free memory/call unload steps
    if (menus[index].unload_callback != NULL) {
        menus[index].unload_callback();
    }
    menu_layer_destroy(menus[index].menu_layer);
    window_destroy(window);

    // reset struct data and decrease number of windows
    menus[index] = (MenuData) {
        .window = NULL,
        .menu_layer = NULL,
        .options = NULL,
        .num_options = 0,
        .click_callback = NULL,
        .unload_callback = NULL
    };
    num_windows--;
}

int16_t add_menu(
    MenuLayerSelectCallback click_callback,
    void (*unload_callback)(),
    char** options,
    uint16_t num_options
) {
    // find a free index to add a menu to in the array
    int16_t index = -1;
    for (int i = 0; i < MAX_MENUS; i++) {
        if (menus[i].window == NULL) {
            index = i;
            break;
        }
    }

    // if index could be found, create window and add to arrays
    if (index != -1) {
        printf("creating menu... index [%d]", index);

        Window* window = window_create();
        window_set_window_handlers(
            window,
            (WindowHandlers) {
                .load = window_load,
                .unload = window_unload
            }
        );

        menus[index].window = window;
        num_windows++;

        menus[index].options = options;
        menus[index].num_options = num_options;

        menus[index].click_callback = click_callback;
        menus[index].unload_callback = unload_callback;
    }

    // returns index or -1 depending if a spot could be found
    return index;
}

Window* get_menu(int16_t index) {
    return menus[index].window;
}

bool menu_is_loaded(int16_t index) {
    // window is not loaded if index out of bounds or array index is null
    if (index < 0 || index >= MAX_MENUS || menus[index].window == NULL) {
        return 0;
    }

    return 1;
}

void menu_window_push(int16_t index) {
    if (index < 0 || index >= MAX_MENUS) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Window push error: index %d out of bounds!", index);
        return;
    }

    if (menus[index].window == NULL) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Window push error: window at index %d is NULL!", index);
        return;
    }

    window_stack_push(menus[index].window, true);
}

void menu_set_highlight_colors(GColor background, GColor foreground) {
    for (int i = 0; i < MAX_MENUS; i++) {
        if (menus[i].menu_layer != NULL) {
            menu_layer_set_highlight_colors(
                menus[i].menu_layer,
                background,
                foreground
            );
        }
    }
}

void menu_mark_dirty() {
    Window* top = window_stack_get_top_window();
    if (top != NULL) {
        Layer* top_layer = window_get_root_layer(top);
        layer_mark_dirty(top_layer);
    }
}
