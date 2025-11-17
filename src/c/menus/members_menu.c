#include "members_menu.h"
#include <pebble.h>
#include "frontable_menu.h"
#include "../data/frontable_cache.h"
#include "../tools/string_tools.h"

static Group root_group;
static FrontableMenu* root_menu;

static Group* groups = NULL;
static FrontableMenu** menus = NULL;

static bool initialized = false;

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    FrontableMenu* menu = (FrontableMenu*)context;
    frontable_menu_draw_cell(menu, ctx, cell_layer, cell_index);
}

static void select(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    FrontableMenu* menu = (FrontableMenu*)context;
    frontable_menu_select(menu, cell_index);
}

static void groups_init() {
    const uint16_t NUM_GROUPS = 3;

    MemberMenuCallbacks callbacks = {
        .draw_row = draw_row,
        .select = select,
        .window_load = NULL,
        .window_unload = NULL
    };

    FrontableList* all_members = cache_get_members();

    static FrontableList list_one;
    frontable_list_add(all_members->frontables[0], &list_one);
    frontable_list_add(all_members->frontables[1], &list_one);
    frontable_list_add(all_members->frontables[2], &list_one);
    static FrontableList list_two;
    frontable_list_add(all_members->frontables[3], &list_two);
    frontable_list_add(all_members->frontables[4], &list_two);
    frontable_list_add(all_members->frontables[5], &list_two);
    frontable_list_add(all_members->frontables[6], &list_two);
    frontable_list_add(all_members->frontables[7], &list_two);
    static FrontableList list_three;
    frontable_list_add(all_members->frontables[8], &list_three);
    frontable_list_add(all_members->frontables[9], &list_three);
    frontable_list_add(all_members->frontables[10], &list_three);

    groups = (Group*)malloc(sizeof(Group) * NUM_GROUPS);
    groups[0] = (Group) {
        .color = GColorRed,
        .name = "RedGroop",
        .parent = &root_group,
        .frontables = &list_one
    };
    groups[1] = (Group) {
        .color = GColorShockingPink,
        .name = "OtherGrop!",
        .parent = &root_group,
        .frontables = &list_two
    };
    groups[2] = (Group) {
        .color = GColorGreen,
        .name = "NestGroup...",
        .parent = &groups[0],
        .frontables = &list_three
    };

    menus = (FrontableMenu**)malloc(sizeof(FrontableMenu*) * NUM_GROUPS);
    menus[0] = frontable_menu_create(callbacks, &groups[0], root_menu);
    menus[1] = frontable_menu_create(callbacks, &groups[1], root_menu);
    menus[2] = frontable_menu_create(callbacks, &groups[2], menus[0]);
}

static void groups_deinit() {
    free(menus);
    free(groups);
}

void members_menu_push() {
    if (!initialized) {
        // set up root group
        root_group.frontables = cache_get_members();
        root_group.color = GColorWhite;
        strcpy(root_group.name, "root");
        root_group.parent = NULL;

        MemberMenuCallbacks callbacks = {
            .draw_row = draw_row,
            .select = select,
            .window_load = NULL,
            .window_unload = NULL
        };

        root_menu = frontable_menu_create(callbacks, &root_group, NULL);

        groups_init();

        initialized = true;
    }

    frontable_menu_window_push(root_menu);
}

void members_menu_deinit() {
    if (initialized) {
        frontable_menu_destroy(root_menu);
        root_menu = NULL;
        groups_deinit();
        initialized = false;
    }
}

void members_menu_update_colors() {
    if (initialized) {
        frontable_menu_update_colors(root_menu);
    }
}
