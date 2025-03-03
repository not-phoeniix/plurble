#include "members_menu.h"
#include "../tools/string_tools.h"
#include "../config/config.h"
#include "../member.h"
#include "main_menu.h"
#include "../messaging/messaging.h"
#include "../tools/drawing.h"

static Window* window = NULL;
static MenuLayer* menu_layer = NULL;
static GColor highlight_color;
static ActionMenuLevel* non_fronting_action_level = NULL;
static ActionMenuLevel* fronting_action_level = NULL;
static ActionMenuConfig action_menu_config;
static Member* selected_member = NULL;

static MemberList* members = NULL;

// ~~~ HELPER FUNCTIONS ~~~

static void update_selected_highlight(uint16_t index) {
    if (settings_get()->member_color_highlight && members->members != NULL) {
        highlight_color = members->members[index]->color;
    } else {
        highlight_color = settings_get()->accent_color;
    }
}

// ~~~ MENU LAYER SETUP ~~~

static void select(MenuLayer* menu_layer, MenuIndex* menu_index, void* context) {
    // prevent crashing if member list is empty
    if (members->num_stored <= 0) {
        return;
    }

    Member* member = members->members[menu_index->row];
    printf("wow! you clicked... [%s]!!", member->name);

    // change popup root level depending on if member is fronting or not
    if (member->fronting) {
        action_menu_config.root_level = fronting_action_level;
    } else {
        action_menu_config.root_level = non_fronting_action_level;
    }

    selected_member = member;
    action_menu_open(&action_menu_config);
}

static uint16_t get_num_rows(MenuLayer* layer, uint16_t section_index, void* ctx) {
    return members->num_stored;
}

static int16_t get_cell_height(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    bool compact = settings_get()->compact_member_list;
    return compact ? 28 : 44;
}

static uint16_t get_num_sections(MenuLayer* menu_layer, void* context) {
    return 1;
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    Member* member = members->members[cell_index->row];
    bool compact = settings_get()->compact_member_list;

    // small color label on member
    graphics_context_set_fill_color(ctx, member->color);
    GRect color_tag_bounds = layer_get_bounds(cell_layer);
    color_tag_bounds.size.w = 3;
    graphics_fill_rect(ctx, color_tag_bounds, 0, GCornerNone);

    // set the new highlight color before draw
    menu_layer_set_highlight_colors(
        menu_layer,
        highlight_color,
        gcolor_legible_over(highlight_color)
    );

    // draw label text itself
    menu_cell_basic_draw(ctx, cell_layer, member->name, compact ? NULL : member->pronouns, NULL);
}

static void selection_changed(MenuLayer* layer, MenuIndex new_index, MenuIndex old_index, void* context) {
    update_selected_highlight(new_index.row);
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
            .get_num_sections = get_num_sections,
            .get_cell_height = get_cell_height,
            .select_click = select,
            .selection_changed = selection_changed
        }
    );

    menu_layer_set_click_config_onto_window(menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
    update_selected_highlight(0);
}

// ~~~ ACTION MENU SETUP ~~~

static void action_set_to_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    if (selected_member != NULL) {
        messaging_set_to_front(selected_member);
    }
}

static void action_add_to_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    if (selected_member != NULL) {
        messaging_add_to_front(selected_member);
    }
}

static void action_remove_from_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    if (selected_member != NULL) {
        messaging_remove_from_front(selected_member);
    }
}

static void action_menu_setup() {
    non_fronting_action_level = action_menu_level_create(2);

    action_menu_level_add_action(non_fronting_action_level, "Set to front", action_set_to_front, NULL);
    action_menu_level_add_action(non_fronting_action_level, "Add to front", action_add_to_front, NULL);

    fronting_action_level = action_menu_level_create(1);
    action_menu_level_add_action(fronting_action_level, "Remove from front", action_remove_from_front, NULL);

    action_menu_config = (ActionMenuConfig) {
        .root_level = non_fronting_action_level,
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
    action_menu_hierarchy_destroy(non_fronting_action_level, NULL, NULL);
    non_fronting_action_level = NULL;
    action_menu_hierarchy_destroy(fronting_action_level, NULL, NULL);
    fronting_action_level = NULL;
    selected_member = NULL;
}

/// ~~~ HEADER FUNCTIONS ~~~

void members_menu_push(MemberList* p_members) {
    members = p_members;

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

void members_menu_update_colors() {
    ClaySettings* settings = settings_get();
    if (settings != NULL) {
        if (menu_layer != NULL) {
            menu_layer_set_highlight_colors(
                menu_layer,
                settings->accent_color,
                gcolor_legible_over(settings->accent_color)
            );
            menu_layer_set_normal_colors(
                menu_layer,
                settings->background_color,
                gcolor_legible_over(settings->background_color)
            );
        }

        if (window != NULL) {
            window_set_background_color(window, settings->background_color);
        }

        action_menu_config.colors.background = settings->accent_color;
    }
}

void members_menu_deinit() {
    if (non_fronting_action_level != NULL) {
        action_menu_hierarchy_destroy(non_fronting_action_level, NULL, NULL);
        non_fronting_action_level = NULL;
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
