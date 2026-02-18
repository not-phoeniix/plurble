#include "frontable_menu.h"
#include "../messaging/messaging.h"

#define TREE_MAX_CHILD_COUNT 64

struct GroupTreeNode;
typedef struct GroupTreeNode {
    Group* group;
    struct GroupTreeNode* parent;
    struct GroupTreeNode* children[TREE_MAX_CHILD_COUNT];
    uint16_t num_children;
    FrontableMenu* menu;
} GroupTreeNode;

struct FrontableMenu {
    Window* window;
    MemberMenuCallbacks callbacks;

    MenuLayer* menu_layer;
    ActionMenuLevel* non_fronting_action_level;
    ActionMenuLevel* fronting_action_level;
    ActionMenuConfig action_menu_config;

    TextLayer* status_bar_text;
    Layer* status_bar_layer;

    GroupTreeNode group_node;

    uint32_t selected_frontable_hash;
    GColor highlight_color;

    uint16_t index_on_load;
};

// ~~~ HELPER FUNCTIONS ~~~

static void update_selected_highlight(FrontableMenu* menu, uint16_t index) {
    GColor color = settings_get_global_accent();

    FrontableList* frontables = menu->group_node.group->frontables;

    if (settings_get()->member_color_highlight) {
        if (index < menu->group_node.num_children) {
            // try to get color of selected group first
            color = menu->group_node.children[index]->group->color;
        } else if (frontables->num_stored > 0) {
            // otherwise try to get color of selected frontable
            int16_t i = index - menu->group_node.num_children;
            if (i >= 0 && i < frontables->num_stored) {
                Frontable* f = frontables->frontables[i];
                color = frontable_get_color(f);
            }
        }
    }

    if (color.argb == settings_get()->background_color.argb) {
        color = gcolor_legible_over(settings_get()->background_color);
    }

    menu->highlight_color = color;
}

static void window_pop_recursive(FrontableMenu* menu, bool pop_root, bool animated) {
    GroupTreeNode* parent = menu->group_node.parent;

    if (parent == NULL) {
        if (pop_root) {
            window_stack_remove(menu->window, animated);
        }
    } else {
        window_stack_remove(menu->window, false);
        window_pop_recursive(parent->menu, pop_root, animated);
    }
}

static void window_push_recursive(FrontableMenu* menu, Window* root_limit) {
    GroupTreeNode* parent = menu->group_node.parent;

    if (parent != NULL && parent->menu->window != root_limit) {
        window_push_recursive(parent->menu, root_limit);
    }

    window_stack_push(menu->window, false);
}

// ~~~ MENU LAYER SETUP ~~~

static uint16_t get_num_rows(MenuLayer* layer, uint16_t section_index, void* context) {
    FrontableMenu* menu = (FrontableMenu*)context;
    FrontableList* frontables = menu->group_node.group->frontables;
    return frontables->num_stored + menu->group_node.num_children;
}

static int16_t get_cell_height(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    bool compact = settings_get()->compact_member_list;

#if defined(PBL_ROUND)
    if (menu_layer_is_index_selected(menu_layer, cell_index)) {
        return compact ? 28 : 50;
    } else {
        return compact ? 28 : 32;
    }
#elif defined(PBL_PLATFORM_EMERY)
    return compact ? 32 : 56;
#else
    return compact ? 28 : 44;
#endif
}

static uint16_t get_num_sections(MenuLayer* menu_layer, void* context) {
    return 1;
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    FrontableMenu* menu = (FrontableMenu*)context;

    Group* group = NULL;
    Frontable* frontable = NULL;

    if (cell_index->row < menu->group_node.num_children) {
        group = menu->group_node.children[cell_index->row]->group;
    } else {
        int16_t i = cell_index->row - menu->group_node.num_children;
        FrontableList* frontables = menu->group_node.group->frontables;
        if (i >= 0 && i < frontables->num_stored) {
            frontable = frontables->frontables[i];
        }
    }

    menu->callbacks.draw_row(menu, ctx, cell_layer, frontable, group);
}

static void selection_changed(MenuLayer* layer, MenuIndex new_index, MenuIndex old_index, void* context) {
    FrontableMenu* menu = (FrontableMenu*)context;

    if (menu != NULL) {
        update_selected_highlight(menu, new_index.row);
    }

    menu->index_on_load = new_index.row;
    menu->selected_frontable_hash = 0;

    int16_t i = new_index.row - menu->group_node.num_children;
    FrontableList* frontables = menu->group_node.group->frontables;
    if (i >= 0 && i < frontables->num_stored) {
        menu->selected_frontable_hash = frontables->frontables[i]->hash;
    }
}

static void status_bar_update_proc(Layer* layer, GContext* ctx) {
    Window* window = layer_get_window(layer);
    FrontableMenu* menu = (FrontableMenu*)window_get_user_data(window);

    GColor bg = settings_get()->background_color;
    if (settings_get()->group_title_accent) {
        bg = menu->group_node.group->color;
    }

    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void menu_layer_setup(FrontableMenu* menu) {
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
            .draw_row = draw_row,
            .get_num_sections = get_num_sections,
            .get_cell_height = get_cell_height,
            .select_click = menu->callbacks.select,
            .selection_changed = selection_changed
        }
    );

    menu_layer_set_click_config_onto_window(menu->menu_layer, menu->window);
    layer_add_child(root_layer, menu_layer_get_layer(menu->menu_layer));
    update_selected_highlight(menu, 0);
    menu_layer_set_selected_index(
        menu->menu_layer,
        (MenuIndex) {.row = menu->index_on_load},
        MenuRowAlignCenter, false
    );
    frontable_menu_update_colors(menu);

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
    text_layer_set_text(menu->status_bar_text, menu->group_node.group->name);

    layer_add_child(menu->status_bar_layer, text_layer_get_layer(menu->status_bar_text));
    layer_add_child(root_layer, menu->status_bar_layer);
}

// ~~~ ACTION MENU SETUP ~~~

static void action_set_as_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    FrontableMenu* menu = (FrontableMenu*)context;
    messaging_set_as_front(menu->selected_frontable_hash);
    window_pop_recursive(menu, true, true);
}

static void action_add_to_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    FrontableMenu* menu = (FrontableMenu*)context;
    messaging_add_to_front(menu->selected_frontable_hash);
}

static void action_remove_from_front(ActionMenu* action_menu, const ActionMenuItem* action, void* context) {
    FrontableMenu* menu = (FrontableMenu*)context;
    messaging_remove_from_front(menu->selected_frontable_hash);
}

static void action_menu_setup(FrontableMenu* menu) {
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
    FrontableMenu* menu = (FrontableMenu*)window_get_user_data(window);

    menu_layer_setup(menu);
    action_menu_setup(menu);

    frontable_menu_update_colors(menu);

    if (menu->callbacks.window_load != NULL) {
        menu->callbacks.window_load(window);
    }
}

static void window_unload(Window* window) {
    FrontableMenu* menu = (FrontableMenu*)window_get_user_data(window);

    menu->index_on_load = 0;

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

void frontable_menu_draw_cell_custom(
    FrontableMenu* menu,
    GContext* ctx,
    const Layer* cell_layer,
    const char* main_text,
    const char* bottom_left_text,
    const char* bottom_right_text,
    GColor tag_color
) {
    GRect bounds = layer_get_bounds(cell_layer);

    if (settings_get()->member_color_tag) {
        // small color label on frontable
        graphics_context_set_fill_color(ctx, tag_color);
        GRect color_tag_bounds = bounds;
#ifdef PBL_PLATFORM_EMERY
        color_tag_bounds.size.w = 6;
#else
        color_tag_bounds.size.w = 3;
#endif
        graphics_fill_rect(ctx, color_tag_bounds, 0, GCornerNone);
    }

    // set the new highlight color before draw
    GColor highlight = frontable_menu_get_current_highlight_color(menu);
    GColor text_color = gcolor_legible_over(highlight);
    menu_layer_set_highlight_colors(
        frontable_menu_get_menu_layer(menu),
        highlight,
        text_color
    );

    // ~~~ DRAW CODE ~~~

    // text parameters
    static const char* NAME_FONT = FONT_KEY_GOTHIC_24_BOLD;
    static const char* SUBTITLE_FONT = FONT_KEY_GOTHIC_18;
#ifdef PBL_PLATFORM_EMERY
    static const uint16_t PADDING = 10;
#else
    static const uint16_t PADDING = 5;
#endif
    static const uint16_t TEXT_SEPARATION = 1;

    // text sizes ...

    // limit text to our padded bounds
    GRect padded_bounds = grect_inset(bounds, GEdgeInsets1(PADDING));

    GSize main_size = graphics_text_layout_get_content_size(
        main_text,
        fonts_get_system_font(NAME_FONT),
        padded_bounds,
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft
    );

    GSize bottom_left_size = {0, 0};
    if (bottom_left_text != NULL) {
        bottom_left_size = graphics_text_layout_get_content_size(
            bottom_left_text,
            fonts_get_system_font(SUBTITLE_FONT),
            padded_bounds,
            GTextOverflowModeTrailingEllipsis,
            GTextAlignmentLeft
        );
    }

    GSize bottom_right_size = {0, 0};
    if (bottom_right_text != NULL) {
        bottom_right_size = graphics_text_layout_get_content_size(
            bottom_right_text,
            fonts_get_system_font(SUBTITLE_FONT),
            padded_bounds,
            GTextOverflowModeTrailingEllipsis,
            GTextAlignmentRight
        );
    }

    // adjust sizes so bottom left and right boxes don't overlap
    if (bottom_left_size.w + bottom_right_size.w > padded_bounds.size.w) {
        bottom_left_size.w = padded_bounds.size.w - bottom_right_size.w;
    }

    // create target drawing boxes
    GRect main_box = {{0, 0}, main_size};
    GRect bottom_left_box = {{0, 0}, bottom_left_size};
    GRect bottom_right_box = {{0, 0}, bottom_right_size};
    if (bottom_left_text == NULL && bottom_right_text == NULL) {
        grect_align(&main_box, &padded_bounds, GAlignLeft, false);
        // center-align
        main_box.origin.y -= (main_size.h / 4);
    } else {
        int16_t middle = padded_bounds.origin.y + (padded_bounds.size.h / 2);

        // main box aligned on top of middle line
        main_box.origin.x = padded_bounds.origin.x;
        main_box.origin.y = middle - main_size.h;

        // bottom boxes aligned right below the middle line
        bottom_left_box.origin.x = padded_bounds.origin.x;
        bottom_left_box.origin.y = middle - (bottom_left_size.h / 4);
        bottom_right_box.origin.x = padded_bounds.origin.x + padded_bounds.size.w - bottom_right_size.w;
        bottom_right_box.origin.y = middle - (bottom_right_size.h / 4);

        // separate text
        main_box.origin.y -= TEXT_SEPARATION;
        bottom_left_box.origin.y += TEXT_SEPARATION;
        bottom_right_box.origin.y += TEXT_SEPARATION;
    }

    // text drawing itself
    graphics_draw_text(
        ctx,
        main_text,
        fonts_get_system_font(NAME_FONT),
        main_box,
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft,
        NULL
    );
    if (bottom_left_text != NULL) {
        graphics_draw_text(
            ctx,
            bottom_left_text,
            fonts_get_system_font(SUBTITLE_FONT),
            bottom_left_box,
            GTextOverflowModeTrailingEllipsis,
            GTextAlignmentLeft,
            NULL
        );
    }
    if (bottom_right_text != NULL) {
        graphics_draw_text(
            ctx,
            bottom_right_text,
            fonts_get_system_font(SUBTITLE_FONT),
            bottom_right_box,
            GTextOverflowModeTrailingEllipsis,
            GTextAlignmentRight,
            NULL
        );
    }
}

void frontable_menu_draw_cell_default(FrontableMenu* menu, GContext* ctx, const Layer* cell_layer, Frontable* selected_frontable, Group* selected_group) {
    bool compact = settings_get()->compact_member_list;

    char* name = NULL;
    char* pronouns = NULL;
    GColor color = GColorBlack;

    if (selected_group != NULL) {
        name = selected_group->name;
        color = selected_group->color;
    } else if (selected_frontable != NULL) {
        color = frontable_get_color(selected_frontable);
        name = selected_frontable->name;
        if (!frontable_get_is_custom(selected_frontable) && selected_frontable->pronouns[0] != '\0') {
            pronouns = selected_frontable->pronouns;
        }
    }

    frontable_menu_draw_cell_custom(
        menu,
        ctx,
        cell_layer,
        name,
        !compact ? pronouns : NULL,
        NULL,
        color
    );
}

static void select_frontable(FrontableMenu* menu, Frontable* frontable) {
    // make accent be the color of the frontable, and change it
    //   if it matches the color of the background
    GColor action_menu_accent = frontable_get_color(frontable);
    if ((action_menu_accent.argb & 0b00111111) == 0) {
        action_menu_accent = GColorDarkGray;
    }
    menu->action_menu_config.colors.background = action_menu_accent;

    // set little dots colors to whatever is legible for new bg color
    menu->action_menu_config.colors.foreground = gcolor_legible_over(
        menu->action_menu_config.colors.background
    );

    // change popup root level depending on if frontable is fronting or not
    if (frontable_get_is_fronting(frontable)) {
        menu->action_menu_config.root_level = menu->fronting_action_level;
    } else {
        menu->action_menu_config.root_level = menu->non_fronting_action_level;
    }

    // change selected frontable and open menu itself
    menu->selected_frontable_hash = frontable->hash;
    action_menu_open(&menu->action_menu_config);
}

static void select_group(FrontableMenu* menu, GroupTreeNode* node) {
    frontable_menu_window_push(node->menu, false, true);
}

void frontable_menu_select(FrontableMenu* menu, MenuIndex* cell_index) {
    if (
        menu->group_node.group->frontables->num_stored <= 0 &&
        menu->group_node.num_children <= 0
    ) {
        return;
    }

    if (cell_index->row < menu->group_node.num_children) {
        GroupTreeNode* node = menu->group_node.children[cell_index->row];
        select_group(menu, node);
    } else {
        uint16_t i = cell_index->row - menu->group_node.num_children;
        Frontable* f = menu->group_node.group->frontables->frontables[i];
        select_frontable(menu, f);
    }
}

void frontable_menu_update_colors(FrontableMenu* menu) {
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

        uint16_t current_index = menu_layer_get_selected_index(menu->menu_layer).row;
        update_selected_highlight(menu, current_index);
    }

    if (menu->status_bar_text != NULL) {
        GColor bg = settings_get()->background_color;
        if (settings_get()->group_title_accent) {
            bg = menu->group_node.group->color;
        }

        GColor fg = gcolor_legible_over(bg);
        text_layer_set_background_color(menu->status_bar_text, bg);
        text_layer_set_text_color(menu->status_bar_text, fg);
    }

    if (menu->window != NULL) {
        window_set_background_color(menu->window, settings->background_color);
    }
}

static void remove_from_parent(GroupTreeNode* node) {
    if (node->parent == NULL) return;

    // find child pointer in parent's children, remove if found
    for (uint16_t i = 0; i < node->parent->num_children; i++) {
        if (node->parent->children[i] == node) {
            // if node is found move everything forward an index
            for (uint16_t j = i + 1; j < node->parent->num_children; j++) {
                node->parent->children[j - 1] = node->parent->children[j];
            }

            node->parent->num_children--;
            break;
        }
    }

    node->parent = NULL;
}

static void add_child_group(GroupTreeNode* parent, GroupTreeNode* child) {
    remove_from_parent(child);

    child->parent = parent;

    uint16_t i = parent->num_children;
    if (i >= TREE_MAX_CHILD_COUNT) {
        APP_LOG(APP_LOG_LEVEL_WARNING, "WARNING: Frontable menu max child count reached!");
        return;
    }

    parent->children[i] = child;
    parent->num_children++;
}

FrontableMenu* frontable_menu_create(MemberMenuCallbacks callbacks, Group* group) {
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
    FrontableMenu* menu = (FrontableMenu*)malloc(sizeof(FrontableMenu));
    *menu = (FrontableMenu) {
        .window = window,
        .callbacks = callbacks,
        .group_node = {
            .group = group,
            .menu = menu,
            .num_children = 0,
            .children = {NULL},
            .parent = NULL
        }
    };

    // set user data of windows to be the related frontable menu pointer
    window_set_user_data(window, menu);

    return menu;
}

void frontable_menu_destroy(FrontableMenu* menu) {
    window_destroy(menu->window);
    free(menu);
}

void frontable_menu_window_push(FrontableMenu* menu, bool recursive, bool animated) {
    // if top of window stack isn't the parent of this menu
    //   AND the menu HAS a parent, push all the parents recursively <3
    GroupTreeNode* parent = menu->group_node.parent;
    Window* current_window = window_stack_get_top_window();
    if (recursive && parent != NULL && parent->menu->window != current_window) {
        window_push_recursive(menu, current_window);
    } else {
        window_stack_push(menu->window, animated);
    }
}

void frontable_menu_window_remove(FrontableMenu* menu) {
    window_stack_remove(menu->window, true);
}

void frontable_menu_window_pop_to_root(FrontableMenu* menu, bool animated) {
    window_pop_recursive(menu, false, animated);
}

FrontableList* frontable_menu_get_frontables(FrontableMenu* menu) {
    return menu->group_node.group->frontables;
}

void frontable_menu_set_frontables(FrontableMenu* menu, FrontableList* frontables) {
    menu->group_node.group->frontables = frontables;
}

void frontable_menu_set_parent(FrontableMenu* menu, FrontableMenu* parent) {
    remove_from_parent(&menu->group_node);
    add_child_group(&parent->group_node, &menu->group_node);
}

void frontable_menu_clear_children(FrontableMenu* menu) {
    menu->group_node.num_children = 0;
}

Window* frontable_menu_get_window(FrontableMenu* menu) {
    return menu->window;
}

MenuIndex frontable_menu_get_selected_index(FrontableMenu* menu) {
    return menu_layer_get_selected_index(menu->menu_layer);
}

MenuLayer* frontable_menu_get_menu_layer(FrontableMenu* menu) {
    return menu->menu_layer;
}

GColor frontable_menu_get_current_highlight_color(FrontableMenu* menu) {
    return menu->highlight_color;
}

void frontable_menu_set_selected_index(FrontableMenu* menu, uint16_t index) {
    menu->index_on_load = index;
    if (menu->menu_layer != NULL) {
        menu_layer_set_selected_index(
            menu->menu_layer,
            (MenuIndex) {.row = index},
            MenuRowAlignCenter,
            false
        );
    }
}

void frontable_menu_clamp_selected_index(FrontableMenu* menu) {
    if (menu->menu_layer == NULL) return;

    MenuIndex i = menu_layer_get_selected_index(menu->menu_layer);
    uint16_t num_frontables = menu->group_node.group->frontables->num_stored;
    uint16_t num_groups = menu->group_node.num_children;
    if (i.row > 0 && i.row >= num_frontables + num_groups) {
        i.row = num_frontables + num_groups - 1;
        menu_layer_set_selected_index(
            menu->menu_layer,
            (MenuIndex) {.row = i.row, .section = i.section},
            MenuRowAlignCenter,
            true
        );
    }
}

const char* frontable_menu_get_name(FrontableMenu* menu) {
    return menu->group_node.group->name;
}
