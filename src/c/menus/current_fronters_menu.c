#include "current_fronters_menu.h"
#include "frontable_menu.h"
#include "../data/frontable_cache.h"

static FrontableMenu* menu = NULL;
static TextLayer* text_layer = NULL;
static Group group;
static bool empty = false;

static void tick_handler(struct tm* tick_time, TimeUnits units_changed) {
    if (menu != NULL) {
        Window* window = frontable_menu_get_window(menu);
        Layer* root = window_get_root_layer(window);
        layer_mark_dirty(root);
    }
}

static void draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* context) {
    // TODO: draw elapsed time fronting for each member

    // frontable_menu_draw_cell(menu, ctx, cell_layer, cell_index);

    GRect bounds = layer_get_bounds(cell_layer);
    bool compact = settings_get()->compact_member_list;

    char* name = NULL;
    // char* pronouns = NULL;
    GColor color = GColorBlack;

    time_t time_now = 0;
    time_ms(&time_now, NULL);

    // HH:MM:SS
    char time_fronting_str[16] = {'\0'};

    uint16_t i = cell_index->row;
    FrontableList* frontables = cache_get_current_fronters();
    if (i < frontables->num_stored) {
        Frontable* frontable = frontables->frontables[i];

        uint32_t diff = time_now - frontable->time_started_fronting;
        uint32_t hours = diff / 60 / 60;
        uint32_t minutes = (diff - (hours * 60 * 60)) / 60;
        uint32_t seconds = (diff - (minutes * 60) - (hours * 60 * 60));
        snprintf(
            time_fronting_str,
            sizeof(time_fronting_str),
            "%lu:%02lu:%02lu",
            hours,
            minutes,
            seconds
        );

        color = frontable_get_color(frontable);
        name = frontable->name;
        if (!frontable_get_is_custom(frontable) && frontable->pronouns[0] != '\0') {
            // pronouns = frontable->pronouns;
        }
    }

    if (settings_get()->member_color_tag) {
        // small color label on frontable
        graphics_context_set_fill_color(ctx, color);
        GRect color_tag_bounds = bounds;
#ifdef PBL_PLATFORM_EMERY
        color_tag_bounds.size.w = 6;
#else
        color_tag_bounds.size.w = 3;
#endif
        graphics_fill_rect(ctx, color_tag_bounds, 0, GCornerNone);
    }

    // draw label text itself
    menu_cell_basic_draw(
        ctx,
        cell_layer,
        name,
        // compact ? NULL : pronouns,
        compact ? NULL : time_fronting_str,
        NULL
    );
}

static void select(MenuLayer* menu_layer, MenuIndex* cell_index, void* context) {
    frontable_menu_select(menu, cell_index);
}

static void window_load(Window* window) {
    Layer* root_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root_layer);
    GRect text_bounds = {
        .origin = (GPoint) {.x = 0, .y = 0},
        .size = {
            .w = 130,
            .h = 80
        }
    };

    grect_align(&text_bounds, &bounds, GAlignCenter, false);

    text_layer = text_layer_create(text_bounds);
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text(text_layer, "No one is currently fronting...");
    text_layer_set_background_color(text_layer, settings_get()->background_color);
    text_layer_set_text_color(text_layer, gcolor_legible_over(settings_get()->background_color));
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);

    if (cache_get_current_fronters()->num_stored <= 0) {
        layer_add_child(root_layer, text_layer_get_layer(text_layer));
        empty = true;
    } else {
        empty = false;
    }

    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void window_unload(Window* window) {
    text_layer_destroy(text_layer);
    text_layer = NULL;

    tick_timer_service_unsubscribe();
}

void current_fronters_menu_push() {
    if (menu == NULL) {
        MemberMenuCallbacks callbacks = {
            .draw_row = draw_row,
            .select = select,
            .window_load = window_load,
            .window_unload = window_unload
        };

        group.color = settings_get()->background_color;
        group.frontables = cache_get_current_fronters();
        strcpy(group.name, "Fronters");
        group.parent = NULL;
        menu = frontable_menu_create(callbacks, &group);
    }

    current_fronters_menu_update_is_empty();
    frontable_menu_window_push(menu, false, true);
}

void current_fronters_menu_deinit() {
    if (menu != NULL) {
        frontable_menu_destroy(menu);
        menu = NULL;
    }
}

void current_fronters_menu_update_colors() {
    group.color = settings_get()->background_color;

    if (menu != NULL) {
        frontable_menu_update_colors(menu);
        frontable_menu_clamp_selected_index(menu);
    }

    if (text_layer != NULL) {
        text_layer_set_background_color(text_layer, settings_get()->background_color);
        text_layer_set_text_color(text_layer, gcolor_legible_over(settings_get()->background_color));
    }
}

void current_fronters_menu_update_is_empty() {
    // don't update anything if text layer doesn't exist lol
    if (text_layer == NULL) {
        return;
    }

    Frontable* first_fronter = cache_get_first_fronter();
    bool current_is_empty = first_fronter == NULL;

    // don't update any states if the empty flag is
    //   already the same as the desired state
    if (empty == current_is_empty) {
        return;
    }

    // get layer pointers
    Window* window = frontable_menu_get_window(menu);
    Layer* root_layer = window_get_root_layer(window);
    Layer* layer = text_layer_get_layer(text_layer);

    // always remove parent so duplicates do not occur, then
    //   if set to show then re-add layer as child of root
    if (current_is_empty) {
        layer_add_child(root_layer, layer);
    } else {
        layer_remove_from_parent(layer);
    }

    // mark to redraw every update, update boolean flag
    empty = current_is_empty;
    layer_mark_dirty(root_layer);
}
