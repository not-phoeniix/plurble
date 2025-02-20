#include "members_menu.h"
#include "menus.h"
#include "../tools/string_tools.h"

static int16_t window_index = -1;

static uint16_t num_members = 0;
static char** members = NULL;

static void click_callback(
    MenuLayer* menu_layer,
    MenuIndex* menu_index,
    void* context
) {
    char* member = members[menu_index->row];
    printf("wow! you clicked... [%s]!!", member);
}

static void unload_callback() {
    if (members != NULL) {
        string_array_free(members, num_members);
    }
}

static void refresh_members() {
    if (members != NULL) {
        string_array_free(members, num_members);
    }

    members = string_split("Names|Go|In|Here|:]", '|', &num_members);
}

void members_menu_push() {
    if (!menu_is_loaded(window_index)) {
        // refresh_members();
        window_index = add_menu(click_callback, unload_callback, members, num_members);
    }

    menu_window_push(window_index);
}

void members_set_members(char* p_members, char delim) {
    if (members != NULL) {
        string_array_free(members, num_members);
    }

    members = string_split(p_members, delim, &num_members);
}