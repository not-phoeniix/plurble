#include "drawing.h"
#include "../config/config.h"

GColor drawing_get_text_color(GColor background) {
    // linear luminance value from 0-1
    float luminance =
        (0.299f * (float)background.r) +
        (0.587f * (float)background.g) +
        (0.114f * (float)background.b);
    luminance /= 2.0f;

    if (luminance >= 0.6f) {
        return GColorBlack;
    } else {
        return GColorWhite;
    }
}

void drawing_draw_member_cell(GContext* ctx, Member* member, MenuLayer* menu_layer, const Layer* cell_layer, GColor highlight_color) {
    GRect bounds = layer_get_bounds(cell_layer);
    bool compact = settings_get()->compact_member_list;

    if (settings_get()->member_color_tag) {
        // small color label on member
        graphics_context_set_fill_color(ctx, member->color);
        GRect color_tag_bounds = bounds;
        color_tag_bounds.size.w = 3;
        graphics_fill_rect(ctx, color_tag_bounds, 0, GCornerNone);
    }

    // set the new highlight color before draw
    menu_layer_set_highlight_colors(
        menu_layer,
        highlight_color,
        gcolor_legible_over(highlight_color)
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

void drawing_deinit() {
}
