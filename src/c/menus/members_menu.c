#include "members_menu.h"
#include <pebble.h>
#include "frontable_menu.h"
#include "../data/frontable_cache.h"
#include "../tools/string_tools.h"

static Group root_group;
static FrontableMenu* root_menu = NULL;
static FrontableList* hidden_root_list = NULL;

static FrontableMenu** menus = NULL;
static uint16_t num_groups = 0;

static bool groups_initialized = false;
static bool root_initialized = false;

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    FrontableMenu* menu = (FrontableMenu*)context;
    frontable_menu_draw_cell(menu, ctx, cell_layer, cell_index);
}

static void select(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    FrontableMenu* menu = (FrontableMenu*)context;
    frontable_menu_select(menu, cell_index);
}

static void root_init() {
    hidden_root_list = frontable_list_create();

    root_group.color = settings_get()->background_color;
    strcpy(root_group.name, "Members");
    root_group.parent = NULL;
    if (settings_get()->hide_members_in_root && settings_get()->show_groups) {
        root_group.frontables = hidden_root_list;
    } else {
        root_group.frontables = cache_get_members();
    }

    MemberMenuCallbacks callbacks = {
        .draw_row = draw_row,
        .select = select,
        .window_load = NULL,
        .window_unload = NULL
    };

    root_menu = frontable_menu_create(callbacks, &root_group);
}

static void find_groupless() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Searching for groupless members...");

    if (hidden_root_list != NULL) {
        frontable_list_clear(hidden_root_list);
    }

    GroupCollection* group_collection = cache_get_groups();

    FrontableList* members = cache_get_members();
    for (uint16_t i = 0; i < members->num_stored; i++) {
        Frontable* member = members->frontables[i];
        bool in_group = false;

        // for each member, search every single group for itself
        for (uint16_t j = 0; j < group_collection->num_stored; j++) {
            Group* group = group_collection->groups[j];
            if (frontable_list_contains(group->frontables, member)) {
                in_group = true;

                // printf("found member '%s' in group '%s' !!", member->name, group->name);

                break;
            }
        }

        if (!in_group) {
            // APP_LOG(APP_LOG_LEVEL_INFO, "Groupless member %s found!", member->name);
            frontable_list_add(member, hidden_root_list);
        }
    }
}

static void groups_init() {
    // checking here too in case groups need to be initialized
    //   before the menu is pushed to the window stack and created
    if (!root_initialized) {
        root_init();
        root_initialized = true;
    }

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
        if (group->parent == NULL) group->parent = &root_group;
        menus[i] = frontable_menu_create(callbacks, group);
    }

    // re-iterate to search for and set parent menus
    for (uint16_t i = 0; i < num_groups; i++) {
        Group* group = group_collection->groups[i];
        FrontableMenu* parent = root_menu;

        for (uint16_t j = 0; j < num_groups; j++) {
            if (group->parent == &root_group) {
                break;
            }

            if (i != j && group->parent == group_collection->groups[j]) {
                parent = menus[j];
            }
        }

        frontable_menu_set_parent(menus[i], parent);
    }
}

static void groups_deinit() {
    if (menus != NULL) {
        for (uint16_t i = 0; i < num_groups; i++) {
            frontable_menu_destroy(menus[i]);
        }

        free(menus);
        menus = NULL;
    }

    if (root_menu != NULL) {
        frontable_menu_clear_children(root_menu);
    }

    if (hidden_root_list != NULL) {
        frontable_list_clear(hidden_root_list);
    }

    num_groups = 0;
}

void members_menu_push() {
    if (!root_initialized) {
        root_init();
        root_initialized = true;
    }

    if (!groups_initialized) {
        groups_init();
        find_groupless();
        groups_initialized = true;
    }

    frontable_menu_window_push(root_menu);
}

void members_menu_deinit() {
    if (root_initialized) {
        frontable_list_destroy(hidden_root_list);
        hidden_root_list = NULL;
        frontable_menu_destroy(root_menu);
        root_menu = NULL;
        root_initialized = false;
    }

    if (groups_initialized) {
        groups_deinit();
        groups_initialized = false;
    }
}

void members_menu_update_colors() {
    root_group.color = settings_get()->background_color;
    if (root_initialized && groups_initialized) {
        frontable_menu_update_colors(root_menu);
        for (uint16_t i = 0; i < num_groups; i++) {
            frontable_menu_update_colors(menus[i]);
        }
    }
}

void members_menu_refresh_groups() {
    if (groups_initialized) {
        groups_deinit();
    }

    groups_init();
    find_groupless();

    if (!settings_get()->show_groups) {
        frontable_menu_clear_children(root_menu);
    }

    if (settings_get()->hide_members_in_root && settings_get()->show_groups) {
        root_group.frontables = hidden_root_list;
    } else {
        root_group.frontables = cache_get_members();
    }

    groups_initialized = true;
}
