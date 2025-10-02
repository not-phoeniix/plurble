#include "member_menu.h"
#include "../messaging/messaging.h"

struct MemberMenu {
    Window* window;
    MemberMenuCallbacks callbacks;

    MenuLayer* menu_layer;
    ActionMenuLevel* non_fronting_action_level;
    ActionMenuLevel* fronting_action_level;
    ActionMenuConfig action_menu_config;

    const char* name;
    TextLayer* status_bar_text;
    Layer* status_bar_layer;

    Frontable* selected_member;
    FrontableList* members;
    GColor highlight_color;
};

// ~~~ HELPER FUNCTIONS ~~~

static void update_selected_highlight(MemberMenu* menu, uint16_t index) {
    if (settings_get()->member_color_highlight && menu->members->members != NULL) {
        menu->highlight_color = menu->members->members[index]->color;
    } else {
        menu->highlight_color = settings_get()->accent_color;
    }
}

// ~~~ MENU LAYER SETUP ~~~

static uint16_t get_num_rows(MenuLayer* layer, uint16_t section_index, void* context) {
    MemberMenu* menu = (MemberMenu*)context;
    if (menu != NULL && menu->members != NULL) {
        return menu->members->num_stored;
    } else {
        return 0;
    }
}

static int16_t get_cell_height(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    bool compact = settings_get()->compact_member_list;

#if defined(PBL_ROUND)
    if (menu_layer_is_index_selected(menu_layer, cell_index)) {
        return compact ? 28 : 50;
    } else {
        return compact ? 28 : 32;
    }
#else
    return compact ? 28 : 44;
#endif
}

static uint16_t get_num_sections(MenuLayer* menu_layer, void* context) {
    return 1;
}

static void selection_changed(MenuLayer* layer, MenuIndex new_index, MenuIndex old_index, void* context) {
    MemberMenu* menu = (MemberMenu*)context;
    menu->selected_member = menu->members->members[new_index.row];
    update_selected_highlight(menu, new_index.row);
}

static void status_bar_update_proc(Layer* layer, GContext* ctx) {
    graphics_context_set_fill_color(ctx, settings_get()->background_color);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void menu_layer_setup(MemberMenu* menu) {
    Layer* root_layer = window_get_root_layer(menu->window);
    GRect bounds = layer_get_bounds(root_layer);

    // ~~~ create menu layers ~~~

#if !defined(PBL_ROUND)
    // only offset if not round, that way round watches
    //   keep the highlighted option centered !
    bounds.size.h -= STATUS_BAR_LAYER_HEIGHT;
    bounds.origin.y += STATUS_BAR_LAYER_HEIGHT;
#endif

    menu->menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(
        menu->menu_layer,
        menu,
        (MenuLayerCallbacks) {
            .get_num_rows = get_num_rows,
            .draw_row = menu->callbacks.draw_row,
            .get_num_sections = get_num_sections,
            .get_cell_height = get_cell_height,
            .select_click = menu->callbacks.select,
            .selection_changed = selection_changed
        }
    );

    menu_layer_set_click_config_onto_window(menu->menu_layer, menu->window);
    layer_add_child(root_layer, menu_layer_get_layer(menu->menu_layer));
    update_selected_highlight(menu, 0);
    member_menu_update_colors(menu);

    // ~~~ create status bar layers ~~~

    GRect status_bar_bounds = layer_get_bounds(root_layer);
    status_bar_bounds.size.h = STATUS_BAR_LAYER_HEIGHT;

    GRect status_bar_text_bounds = status_bar_bounds;
    status_bar_text_bounds.size.h = 14;
    grect_align(&status_bar_text_bounds, &status_bar_bounds, GAlignCenter, false);

#if !defined(PBL_ROUND)
    // shift status bar text up 3 pixels for non round watches!
    status_bar_text_bounds.origin.y -= 3;
#endif

    menu->status_bar_layer = layer_create(status_bar_bounds);
    layer_set_update_proc(menu->status_bar_layer, status_bar_update_proc);

    menu->status_bar_text = text_layer_create(status_bar_text_bounds);
    text_layer_set_font(menu->status_bar_text, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(menu->status_bar_text, GTextAlignmentCenter);
    text_layer_set_text(menu->status_bar_text, menu->name);

    layer_add_child(menu->status_bar_layer, text_layer_get_layer(menu->status_bar_text));
    layer_add_child(root_layer, menu->status_bar_layer);
}

// ~~~ ACTION MENU SETUP ~~~

static void action_set_as_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    MemberMenu* menu = (MemberMenu*)context;
    if (menu->selected_member != NULL) {
        messaging_set_as_front(menu->selected_member);
        window_stack_remove(menu->window, true);
    }
}

static void action_add_to_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    MemberMenu* menu = (MemberMenu*)context;
    if (menu->selected_member != NULL) {
        messaging_add_to_front(menu->selected_member);
    }
}

static void action_remove_from_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    MemberMenu* menu = (MemberMenu*)context;
    if (menu->selected_member != NULL) {
        messaging_remove_from_front(menu->selected_member);
    }
}

static void action_menu_setup(MemberMenu* menu) {
    menu->non_fronting_action_level = action_menu_level_create(2);

    action_menu_level_add_action(menu->non_fronting_action_level, "Set as front", action_set_as_front, NULL);
    action_menu_level_add_action(menu->non_fronting_action_level, "Add to front", action_add_to_front, NULL);

    menu->fronting_action_level = action_menu_level_create(1);
    action_menu_level_add_action(menu->fronting_action_level, "Remove from front", action_remove_from_front, NULL);

    menu->action_menu_config = (ActionMenuConfig) {
        .root_level = menu->non_fronting_action_level,
        .align = ActionMenuAlignTop,
        .context = menu,
        .colors = {
            .background = settings_get()->accent_color,
            .foreground = GColorBlack
        },
    };
}

// ~~~ LOAD & UNLOAD ~~~

static void window_load(Window* window) {
    MemberMenu* menu = (MemberMenu*)window_get_user_data(window);

    menu_layer_setup(menu);
    action_menu_setup(menu);

    member_menu_update_colors(menu);

    if (menu->callbacks.window_load != NULL) {
        menu->callbacks.window_load(window);
    }
}

static void window_unload(Window* window) {
    MemberMenu* menu = (MemberMenu*)window_get_user_data(window);

    menu_layer_destroy(menu->menu_layer);
    menu->menu_layer = NULL;
    layer_destroy(menu->status_bar_layer);
    menu->status_bar_layer = NULL;
    text_layer_destroy(menu->status_bar_text);
    menu->status_bar_text = NULL;
    action_menu_hierarchy_destroy(menu->fronting_action_level, NULL, NULL);
    menu->fronting_action_level = NULL;
    action_menu_hierarchy_destroy(menu->non_fronting_action_level, NULL, NULL);
    menu->non_fronting_action_level = NULL;

    if (menu->callbacks.window_unload != NULL) {
        menu->callbacks.window_unload(window);
    }
}

// ~~~ PUBLIC HELPERS ~~~

void member_menu_draw_cell(MemberMenu* menu, GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index) {
    GRect bounds = layer_get_bounds(cell_layer);
    bool compact = settings_get()->compact_member_list;
    Frontable* member = menu->members->members[cell_index->row];

    if (settings_get()->member_color_tag) {
        // small color label on member
        graphics_context_set_fill_color(ctx, member->color);
        GRect color_tag_bounds = bounds;
        color_tag_bounds.size.w = 3;
        graphics_fill_rect(ctx, color_tag_bounds, 0, GCornerNone);
    }

    // set the new highlight color before draw
    menu_layer_set_highlight_colors(
        menu->menu_layer,
        menu->highlight_color,
        gcolor_legible_over(menu->highlight_color)
    );

    // draw label text itself
    menu_cell_basic_draw(
        ctx,
        cell_layer,
        member->name,
        compact || member->custom ? NULL : member->pronouns,
        NULL
    );
}

void member_menu_select_member(MemberMenu* menu, MenuIndex* cell_index) {
    // do not select anything if no members are stored!
    if (menu->members->num_stored <= 0) {
        return;
    }

    Frontable* member = menu->members->members[cell_index->row];

    // set background/bar color of action menu to either member
    //   color or global accent depending on prefs
    if (settings_get()->member_color_highlight) {
        menu->action_menu_config.colors.background = member->color;
    } else {
        menu->action_menu_config.colors.background = settings_get_global_accent();
    }

    // set little dots colors to whatever is legible for new bg color
    menu->action_menu_config.colors.foreground = gcolor_legible_over(
        menu->action_menu_config.colors.background
    );

    // change popup root level depending on if member is fronting or not
    if (member->fronting) {
        menu->action_menu_config.root_level = menu->fronting_action_level;
    } else {
        menu->action_menu_config.root_level = menu->non_fronting_action_level;
    }

    // change selected member and open menu itself
    menu->selected_member = member;
    action_menu_open(&menu->action_menu_config);
}

void member_menu_update_colors(MemberMenu* menu) {
    ClaySettings* settings = settings_get();

    if (menu->menu_layer != NULL) {
        menu_layer_set_highlight_colors(
            menu->menu_layer,
            settings_get_global_accent(),
            gcolor_legible_over(settings_get_global_accent())
        );
        menu_layer_set_normal_colors(
            menu->menu_layer,
            settings->background_color,
            gcolor_legible_over(settings->background_color)
        );
    }

    if (menu->status_bar_text != NULL) {
        text_layer_set_background_color(menu->status_bar_text, settings->background_color);
        text_layer_set_text_color(menu->status_bar_text, gcolor_legible_over(settings->background_color));
    }

    if (menu->window != NULL) {
        window_set_background_color(menu->window, settings->background_color);
    }
}

MemberMenu* member_menu_create(MemberMenuCallbacks callbacks, FrontableList* members, const char* name) {
    // create window and set up handlers
    Window* window = window_create();
    window_set_window_handlers(
        window,
        (WindowHandlers) {
            .load = window_load,
            .unload = window_unload
        }
    );

    // malloc new menu and assign values
    MemberMenu* menu = (MemberMenu*)malloc(sizeof(MemberMenu));
    *menu = (MemberMenu) {
        .window = window,
        .callbacks = callbacks,
        .members = members,
        .name = name
    };

    // set user data of windows to be the related member menu pointer
    window_set_user_data(window, menu);

    return menu;
}

void member_menu_destroy(MemberMenu* menu) {
    window_destroy(menu->window);
    free(menu);
}

void member_menu_window_push(MemberMenu* menu) {
    window_stack_push(menu->window, true);
}

void member_menu_window_remove(MemberMenu* menu) {
    window_stack_remove(menu->window, true);
}

FrontableList* member_menu_get_members(MemberMenu* menu) {
    return menu->members;
}

void member_menu_set_members(MemberMenu* menu, FrontableList* members) {
    menu->members = members;
}

Window* member_menu_get_window(MemberMenu* menu) {
    return menu->window;
}
