#include "fronters_menu.h"
#include <pebble.h>
#include "../members.h"

static Window* window = NULL;
// static MenuLayer*

static Member** fronters;
static uint16_t num_fronters;

static void window_load() {
}

static void window_unload() {
}

void fronters_menu_push() {
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
