#include "setup_prompt_menu.h"
#include "../data/config.h"

static Window* window = NULL;
static Layer* layer;
static GBitmap* phone_bitmap;
static BitmapLayer* bitmap_layer;

static void draw(Layer* layer, GContext* ctx) {
    GRect bounds = layer_get_bounds(layer);

    GRect text_bounds = bounds;
    text_bounds.size.h = 30;
    grect_align(&text_bounds, &bounds, GAlignCenter, false);
    text_bounds.origin.y += 50;

    graphics_context_set_text_color(
        ctx,
        gcolor_legible_over(settings_get()->accent_color)
    );

    graphics_draw_text(
        ctx,
        "setup in phone...",
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
        text_bounds,
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentCenter,
        NULL
    );
}

static void window_load() {
    Layer* root_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root_layer);

    layer = layer_create(bounds);
    layer_set_update_proc(layer, draw);
    layer_add_child(root_layer, layer);

    phone_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ConfigRequired);
    GRect bitmap_rect = gbitmap_get_bounds(phone_bitmap);
    grect_align(&bitmap_rect, &bounds, GAlignCenter, false);
    bitmap_rect.origin.y -= 15;

    bitmap_layer = bitmap_layer_create(bitmap_rect);
    bitmap_layer_set_bitmap(bitmap_layer, phone_bitmap);
    bitmap_layer_set_compositing_mode(bitmap_layer, GCompOpSet);
    layer_add_child(root_layer, bitmap_layer_get_layer(bitmap_layer));
}

static void window_unload() {
    layer_destroy(layer);
    gbitmap_destroy(phone_bitmap);
    bitmap_layer_destroy(bitmap_layer);
}

void setup_prompt_menu_push() {
    window = window_create();
    window_set_window_handlers(
        window,
        (WindowHandlers) {
            .load = window_load,
            .unload = window_unload
        }
    );
    window_set_background_color(window, settings_get()->accent_color);

    window_stack_push(window, true);
}

void setup_prompt_menu_remove() {
    if (window != NULL) {
        window_stack_remove(window, false);
        window_destroy(window);
        window = NULL;
    }
}

bool setup_prompt_menu_shown() {
    return window_stack_get_top_window() == window;
}
