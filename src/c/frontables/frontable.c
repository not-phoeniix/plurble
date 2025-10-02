#include "frontable.h"

#define NUM_COLORS 64

static uint8_t colors[NUM_COLORS] = {
    GColorBlackARGB8,
    GColorOxfordBlueARGB8,
    GColorDukeBlueARGB8,
    GColorBlueARGB8,
    GColorDarkGreenARGB8,
    GColorMidnightGreenARGB8,
    GColorCobaltBlueARGB8,
    GColorBlueMoonARGB8,
    GColorIslamicGreenARGB8,
    GColorJaegerGreenARGB8,
    GColorTiffanyBlueARGB8,
    GColorVividCeruleanARGB8,
    GColorGreenARGB8,
    GColorMalachiteARGB8,
    GColorMediumSpringGreenARGB8,
    GColorCyanARGB8,
    GColorBulgarianRoseARGB8,
    GColorImperialPurpleARGB8,
    GColorIndigoARGB8,
    GColorElectricUltramarineARGB8,
    GColorArmyGreenARGB8,
    GColorDarkGrayARGB8,
    GColorLibertyARGB8,
    GColorVeryLightBlueARGB8,
    GColorKellyGreenARGB8,
    GColorMayGreenARGB8,
    GColorCadetBlueARGB8,
    GColorPictonBlueARGB8,
    GColorBrightGreenARGB8,
    GColorScreaminGreenARGB8,
    GColorMediumAquamarineARGB8,
    GColorElectricBlueARGB8,
    GColorDarkCandyAppleRedARGB8,
    GColorJazzberryJamARGB8,
    GColorPurpleARGB8,
    GColorVividVioletARGB8,
    GColorWindsorTanARGB8,
    GColorRoseValeARGB8,
    GColorPurpureusARGB8,
    GColorLavenderIndigoARGB8,
    GColorLimerickARGB8,
    GColorBrassARGB8,
    GColorLightGrayARGB8,
    GColorBabyBlueEyesARGB8,
    GColorSpringBudARGB8,
    GColorInchwormARGB8,
    GColorMintGreenARGB8,
    GColorCelesteARGB8,
    GColorRedARGB8,
    GColorFollyARGB8,
    GColorFashionMagentaARGB8,
    GColorMagentaARGB8,
    GColorOrangeARGB8,
    GColorSunsetOrangeARGB8,
    GColorBrilliantRoseARGB8,
    GColorShockingPinkARGB8,
    GColorChromeYellowARGB8,
    GColorRajahARGB8,
    GColorMelonARGB8,
    GColorRichBrilliantLavenderARGB8,
    GColorYellowARGB8,
    GColorIcterineARGB8,
    GColorPastelYellowARGB8,
    GColorWhiteARGB8
};

/*

Frontable* frontable_create(uint32_t hash, const char* name, const char* pronouns, GColor color, bool is_custom) {
    Frontable* frontable = malloc(sizeof(Frontable));
    *frontable = (Frontable) {
        .hash = hash,
        .name = {'\0'},
        .pronouns = {'\0'},
        .color = color,
        .fronting = false,
        .custom = is_custom
    };

    const char* c = name[0];
    uint16_t i = 0;
    while (*c != '\0' && i < FRONTABLE_STRING_SIZE - 1) {
        frontable->name[i] = *c;
        c++;
        i++;
    }

    c = pronouns[0];
    i = 0;
    while (*c != '\0' && i < FRONTABLE_STRING_SIZE - 1) {
        frontable->pronouns[i] = *c;
        c++;
        i++;
    }

    return frontable;
}

void frontable_delete(Frontable* frontable) {
    free(frontable);
}

*/

static void copy_smaller_str(char* dest, const char* src, uint16_t dest_len) {
    for (uint16_t i = 0; i < dest_len; i++) {
        if (i < dest_len - 1) {
            char c = src[i];
            dest[i] = c;

            if (c == '\0') {
                break;
            }
        } else {
            dest[i] = '\0';
        }
    }
}

Frontable* frontable_create(uint32_t hash, const char* name, const char* pronouns, bool is_custom, GColor color) {
    Frontable* f = malloc(sizeof(Frontable));
    *f = (Frontable) {
        .hash = hash,
        .packed_data = frontable_make_packed_data(false, is_custom, color)
    };
    copy_smaller_str(f->name, name, FRONTABLE_STRING_SIZE);
    copy_smaller_str(f->pronouns, pronouns, FRONTABLE_STRING_SIZE);

    return f;
}

uint8_t frontable_make_packed_data(bool fronting, bool is_custom, GColor color) {
    uint8_t data = 0;
    if (fronting) { data |= 0b10000000; }
    if (is_custom) { data |= 0b01000000; }

    uint8_t index = 0;
    for (uint8_t i = 0; i < NUM_COLORS; i++) {
        if (color.argb == colors[i]) {
            index = i;
            break;
        }
    }
    data |= index;

    return data;
}

bool frontable_get_is_custom(const Frontable* frontable) {
    return (frontable->packed_data & 0b01000000) != 0;
}

bool frontable_get_is_fronting(const Frontable* frontable) {
    return (frontable->packed_data & 0b10000000) != 0;
}

void frontable_set_is_fronting(Frontable* frontable, bool fronting) {
    if (frontable_get_is_fronting(frontable) != fronting) {
        frontable->packed_data ^= 0b10000000;
    }
}

GColor frontable_get_color(const Frontable* frontable) {
    uint8_t index = (frontable->packed_data & 0b00111111);
    return (GColor) {.argb = colors[index]};
}
