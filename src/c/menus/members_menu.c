#include "members_menu.h"
#include "../tools/string_tools.h"
#include "../config/config.h"
#include "member.h"

#define MAX_MEMBERS 128

static Window* window = NULL;
static MenuLayer* menu_layer = NULL;
static uint16_t num_members = 0;
static ActionMenuLevel* member_menu_level = NULL;
// static char** members = NULL;
static Member** members = NULL;

static void
select(
    MenuLayer* menu_layer,
    MenuIndex* menu_index,
    void* context
) {
    Member* member = members[menu_index->row];
    printf("wow! you clicked... [%s]!!", member->name);

    ActionMenuConfig config = {
        .root_level = member_menu_level,
        .align = ActionMenuAlignTop,
        .colors = {
            .background = settings_get()->accent_color,
            .foreground = GColorBlack
        }
    };
    action_menu_open(&config);
}

static uint16_t get_num_rows(MenuLayer* layer, uint16_t selected_index, void* ctx) {
    return num_members;
}

static int16_t get_cell_height(struct MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    return 44;
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    Member* member = members[cell_index->row];
    menu_cell_basic_draw(ctx, cell_layer, member->name, member->pronouns, NULL);
}

static void window_load() {
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // ~~~ set up menu layer ~~~

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

    // ~~~ set up action menu level ~~~

    member_menu_level = action_menu_level_create(5);
    action_menu_level_add_action(member_menu_level, "wow", NULL, NULL);
    action_menu_level_add_action(member_menu_level, "wow", NULL, NULL);
    action_menu_level_add_action(member_menu_level, "wow", NULL, NULL);

    members_menu_update_colors();
}

static void window_unload() {
    menu_layer_destroy(menu_layer);
    menu_layer = NULL;
    action_menu_hierarchy_destroy(member_menu_level, NULL, NULL);
    member_menu_level = NULL;
}

static void free_members_arr() {
    for (int i = 0; i < num_members; i++) {
        member_delete(members[i]);
    }

    free(members);
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
