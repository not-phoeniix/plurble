#pragma once

#include <pebble.h>

#define FRONTABLE_NAME_LENGTH 32
#define FRONTABLE_PRONOUNS_LENGTH 16

/// @brief A struct that describes a frontable in a plural system, either a member or a custom front
typedef struct {
    char name[FRONTABLE_NAME_LENGTH + 1];
    char pronouns[FRONTABLE_PRONOUNS_LENGTH + 1];
    uint32_t hash;

    // bits are as follows (left to right):
    //   0: whether or not frontable is fronting
    //   1: whether or not frontable is a custom front
    //   2-7: color index
    uint8_t packed_data;
} Frontable;

/// @brief Creates a new Frontable on the heap
/// @param hash Unique hash of this frontable
/// @param name Name of frontable
/// @param pronouns Pronouns of frontable
/// @param is_custom Whether or not frontable is a custom front
/// @param color Color of frontable
/// @return A pointer to a new Frontable allocated on the heap
Frontable* frontable_create(uint32_t hash, const char* name, const char* pronouns, bool is_custom, GColor color);

/// @brief Creates a packed 8-bit unsigned integer used for frontable data storing/compression
/// @param fronting Whether or not frontable is currently fronting
/// @param is_custom Whether or not frontable is custom
/// @param color Color of frontable
/// @return A packed 8-bit unsigned integer containing data
uint8_t frontable_make_packed_data(bool fronting, bool is_custom, GColor color);

/// @brief Gets whether or not a frontable is a custom front
/// @param frontable Frontable to check data for
/// @return True if custom, false if otherwise
bool frontable_get_is_custom(const Frontable* frontable);

/// @brief Gets whether or not a frontable is currently fronting
/// @param frontable Frontable to check data for
/// @return True if fronting, false if otherwise
bool frontable_get_is_fronting(const Frontable* frontable);

/// @brief Sets whether or not a frontable is currently fronting
/// @param frontable Frontable to set data in
/// @param fronting Whether or not frontable should be fronting
void frontable_set_is_fronting(Frontable* frontable, bool fronting);

/// @brief Gets the color of a frontable
/// @param frontable Frontable to get color from
/// @return Color of frontable
GColor frontable_get_color(const Frontable* frontable);
