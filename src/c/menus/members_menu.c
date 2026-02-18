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

//! you should make this not name-based, save group
//!   UIDs/hashes if possible in the future
static char prev_group_name[GROUP_NAME_LENGTH] = {'\0'};
static uint16_t prev_selected_index = 0;

static void select(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    FrontableMenu* menu = (FrontableMenu*)context;
    frontable_menu_select(menu, cell_index);
}

static void draw_cell(FrontableMenu* menu, GContext* ctx, const Layer* cell_layer, Frontable* selected_frontable, Group* selected_group) {
    bool compact = settings_get()->compact_member_list;
    bool show_pronouns = settings_get()->show_pronouns;

    char* name = NULL;
    char* pronouns = NULL;
    GColor color = GColorBlack;

    if (selected_group != NULL) {
        name = selected_group->name;
        color = selected_group->color;
    } else if (selected_frontable != NULL) {
        color = frontable_get_color(selected_frontable);
        name = selected_frontable->name;
        if (selected_frontable->pronouns[0] != '\0') {
            pronouns = selected_frontable->pronouns;
        }
    }

    frontable_menu_draw_cell_custom(
        menu,
        ctx,
        cell_layer,
        name,
        (!compact && show_pronouns) ? pronouns : NULL,
        NULL,
        color
    );
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
        .draw_row = draw_cell,
        .select = select,
        .window_load = NULL,
        .window_unload = NULL
    };

    root_menu = frontable_menu_create(callbacks, &root_group);
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
        .draw_row = draw_cell,
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
        groups_initialized = true;
    }

    frontable_menu_window_push(root_menu, false, true);
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

void members_menu_remove_groups() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Removing members menu groups...");

    strncpy(prev_group_name, "", sizeof(prev_group_name));
    prev_selected_index = 0;

    if (groups_initialized) {
        FrontableMenu* shown_menu = NULL;

        APP_LOG(APP_LOG_LEVEL_DEBUG, "Searching for a group menu to restore after refresh...");

        // find shown window if any are shown
        Window* top_window = window_stack_get_top_window();
        for (uint16_t i = 0; i < num_groups; i++) {
            if (frontable_menu_get_window(menus[i]) == top_window) {
                shown_menu = menus[i];
                break;
            }
        }

        // save menu name, remove menu because it's about to be destroyed
        if (shown_menu != NULL) {
            APP_LOG(
                APP_LOG_LEVEL_DEBUG,
                "Found group menu '%s'",
                frontable_menu_get_name(shown_menu)
            );

            string_safe_copy(
                prev_group_name,
                frontable_menu_get_name(shown_menu),
                sizeof(prev_group_name)
            );

            prev_selected_index = frontable_menu_get_selected_index(shown_menu).row;
            printf("prev selected index to attempt to restore: %u", prev_selected_index);

            frontable_menu_window_pop_to_root(shown_menu, false);
        } else {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Group menu not identified, members menu must not be pushed to window stack.");
        }

        groups_deinit();
    }

    groups_initialized = false;

    APP_LOG(APP_LOG_LEVEL_INFO, "Groups removed!");
}

void members_menu_create_groups() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Creating members menu groups...");

    groups_init();

    if (!settings_get()->show_groups) {
        frontable_menu_clear_children(root_menu);
    }

    if (settings_get()->hide_members_in_root && settings_get()->show_groups) {
        root_group.frontables = hidden_root_list;
    } else {
        root_group.frontables = cache_get_members();
    }

    // find group again and push it back to the stack (if it exists)
    FrontableMenu* menu_to_restore = NULL;
    for (uint16_t i = 0; i < num_groups; i++) {
        // don't check if a group was never saved
        if (prev_group_name[0] == '\0') break;

        const char* name = frontable_menu_get_name(menus[i]);
        if (string_start_same(name, prev_group_name)) {
            menu_to_restore = menus[i];
            break;
        }
    }

    if (menu_to_restore != NULL) {
        APP_LOG(
            APP_LOG_LEVEL_DEBUG,
            "Found new group menu '%s' to restore!",
            frontable_menu_get_name(menu_to_restore)
        );

        printf("trying to restore menu with index: %u", prev_selected_index);
        frontable_menu_set_selected_index(menu_to_restore, prev_selected_index);
        frontable_menu_window_push(menu_to_restore, true, false);
    }

    groups_initialized = true;

    APP_LOG(APP_LOG_LEVEL_INFO, "Groups created!");
}

void members_menu_refresh_groupless_members() {
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
                break;
            }
        }

        if (!in_group) {
            APP_LOG(APP_LOG_LEVEL_INFO, "Groupless member %s found!", member->name);
            frontable_list_add(member, hidden_root_list);
        }
    }

    APP_LOG(APP_LOG_LEVEL_INFO, "Groupless member search finished!");
}
