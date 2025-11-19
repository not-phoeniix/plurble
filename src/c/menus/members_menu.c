#include "members_menu.h"
#include <pebble.h>
#include "frontable_menu.h"
#include "../data/frontable_cache.h"
#include "../tools/string_tools.h"

static Group root_group;
static FrontableMenu* root_menu;

static FrontableMenu** menus = NULL;
static uint16_t num_groups = 0;

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
    GroupCollection* group_collection = cache_get_groups();
    num_groups = group_collection->num_stored;

    MemberMenuCallbacks callbacks = {
        .draw_row = draw_row,
        .select = select,
        .window_load = NULL,
        .window_unload = NULL
    };

    // create menus with null parent pointers
    menus = (FrontableMenu**)malloc(sizeof(FrontableMenu*) * num_groups);
    for (uint16_t i = 0; i < num_groups; i++) {
        Group* group = group_collection->groups[i];
        menus[i] = frontable_menu_create(callbacks, group);
    }

    // re-iterate to search for and set parent menus
    for (uint16_t i = 0; i < num_groups; i++) {
        Group* group = group_collection->groups[i];
        FrontableMenu* parent = root_menu;

        for (uint16_t j = 0; j < num_groups; j++) {
            if (group->parent == NULL) break;

            if (i != j && group->parent == group_collection->groups[j]) {
                parent = menus[j];
            }
        }

        frontable_menu_set_parent(menus[i], parent);
    }
}

static void groups_deinit() {
    if (menus != NULL) {
        free(menus);
        menus = NULL;
    }
}

void members_menu_push() {
    if (!initialized) {
        // set up root group
        root_group.frontables = cache_get_members();
        root_group.color = settings_get()->background_color;
        strcpy(root_group.name, "Members");
        root_group.parent = NULL;

        MemberMenuCallbacks callbacks = {
            .draw_row = draw_row,
            .select = select,
            .window_load = NULL,
            .window_unload = NULL
        };

        root_menu = frontable_menu_create(callbacks, &root_group);

        cache_clear_groups();

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

        Group* g1 = malloc(sizeof(Group));
        *g1 = (Group) {
            .color = GColorRed,
            .name = "RedGroop",
            .parent = &root_group,
            .frontables = &list_one
        };
        cache_add_group(g1);

        Group* g2 = malloc(sizeof(Group));
        *g2 = (Group) {
            .color = GColorShockingPink,
            .name = "OtherGrop!",
            .parent = &root_group,
            .frontables = &list_two
        };
        cache_add_group(g2);

        Group* g3 = malloc(sizeof(Group));
        *g3 = (Group) {
            .color = GColorGreen,
            .name = "NestGroup...",
            .parent = g1,
            .frontables = &list_three
        };
        cache_add_group(g3);

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
    root_group.color = settings_get()->background_color;
    if (initialized) {
        frontable_menu_update_colors(root_menu);
        for (uint16_t i = 0; i < num_groups; i++) {
            printf("updating menu colors.. %u", i);
            frontable_menu_update_colors(menus[i]);
        }
    }
}

void members_menu_refresh_groups() {
    if (initialized) {
        groups_deinit();
    }

    groups_init();
}
