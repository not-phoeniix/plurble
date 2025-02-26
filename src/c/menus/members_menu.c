#include "members_menu.h"
#include "../tools/string_tools.h"
#include "../config/config.h"
#include "member.h"

static Window* window = NULL;
static MenuLayer* menu_layer = NULL;
static uint16_t num_members = 0;
static ActionMenuLevel* member_menu_level = NULL;
static Member** members = NULL;
static ActionMenuConfig action_menu_config;

// ~~~ HELPER FUNCTIONS ~~~

static void free_members_arr() {
    for (int i = 0; i < num_members; i++) {
        member_delete(members[i]);
    }

    free(members);
}

// ~~~ MENU LAYER SETUP ~~~

static void select(MenuLayer* menu_layer, MenuIndex* menu_index, void* context) {
    Member* member = members[menu_index->row];
    printf("wow! you clicked... [%s]!!", member->name);

    action_menu_open(&action_menu_config);
}

static uint16_t get_num_rows(MenuLayer* layer, uint16_t selected_index, void* ctx) {
    return num_members;
}

static int16_t get_cell_height(struct MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    if (settings_get()->compact_member_list) {
        return 28;
    } else {
        return 44;
    }
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    Member* member = members[cell_index->row];

    if (settings_get()->compact_member_list) {
        menu_cell_basic_draw(ctx, cell_layer, member->name, NULL, NULL);
    } else {
        menu_cell_basic_draw(ctx, cell_layer, member->name, member->pronouns, NULL);
    }
}

static void menu_layer_setup() {
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

    menu_layer_set_click_config_onto_window(menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

// ~~~ ACTION MENU SETUP ~~~

static void action_set_to_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    printf("setting to front...");
}

static void action_add_to_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    printf("adding to front...");
}

static void action_remove_from_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    printf("removing from front...");
}

static void action_menu_setup() {
    member_menu_level = action_menu_level_create(5);

    action_menu_level_add_action(member_menu_level, "Set to front", action_set_to_front, NULL);
    action_menu_level_add_action(member_menu_level, "Add to front", action_add_to_front, NULL);

    action_menu_config = (ActionMenuConfig) {
        .root_level = member_menu_level,
        .align = ActionMenuAlignTop,
        .colors = {
            .background = settings_get()->accent_color,
            .foreground = GColorBlack
        }
    };
}

// ~~~ WINDOW SETUP ~~~

static void window_load() {
    menu_layer_setup();
    action_menu_setup();
    members_menu_update_colors();
}

static void window_unload() {
    menu_layer_destroy(menu_layer);
    menu_layer = NULL;
    action_menu_hierarchy_destroy(member_menu_level, NULL, NULL);
    member_menu_level = NULL;
}

/// ~~~ HEADER FUNCTIONS ~~~

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
        free_members_arr(members);
    }

    // split array by delimiter
    char** member_split = string_split(p_members, delim, &num_members);

    // allocate memory for array of member pointers !
    //   then fill array with members
    members = malloc(sizeof(Member*) * num_members);
    for (uint16_t i = 0; i < num_members; i++) {
        members[i] = member_create(member_split[i]);
    }

    // free previous array split
    string_array_free(member_split, num_members);
}

void members_menu_update_colors() {
    ClaySettings* settings = settings_get();
    if (settings != NULL) {
        if (menu_layer != NULL) {
            menu_layer_set_highlight_colors(
                menu_layer,
                settings->accent_color,
                GColorWhite
            );
        }

        action_menu_config.colors.background = settings->accent_color;
    }
}

void members_menu_deinit() {
    if (members != NULL) {
        free_members_arr();
        members = NULL;
    }

    if (member_menu_level != NULL) {
        action_menu_hierarchy_destroy(member_menu_level, NULL, NULL);
        member_menu_level = NULL;
    }

    if (menu_layer != NULL) {
        menu_layer_destroy(menu_layer);
        menu_layer = NULL;
    }

    if (window != NULL) {
        window_destroy(window);
        window = NULL;
    }
}
