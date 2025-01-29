#include "main_menu.h"
#include "menus.h"
#include "members_menu.h"
#include "../tools/string_tools.h"

static int16_t window_index = -1;
static char* options[] = {
    "Members"
};

static void click_callback(
    MenuLayer* menu_layer,
    MenuIndex* menu_index,
    void* context
) {
    switch (menu_index->row) {
        case 0:
            members_menu_push();
            break;
    }
}

void main_menu_push() {
    if (!menu_is_loaded(window_index)) {
        window_index = add_menu(click_callback, NULL, options, 1);
    }

    menu_window_push(window_index);
}
