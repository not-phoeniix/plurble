#include "drawing.h"

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
