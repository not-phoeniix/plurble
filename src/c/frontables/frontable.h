#pragma once

#include <pebble.h>

#define FRONTABLE_STRING_SIZE 32

/// @brief A struct that describes a frontable in a plural system, either a member or a custom front
typedef struct {
    char name[FRONTABLE_STRING_SIZE];
    char pronouns[FRONTABLE_STRING_SIZE];
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
/// @param color Color of frontable
/// @param is_custom Whether or not frontable is a custom front
/// @return A pointer to a new Frontable object allocated on the heap
// Frontable* frontable_create(
//     uint32_t hash,
//     const char* name,
//     const char* pronouns,
//     GColor color,
//     bool is_custom
// );

/// @brief Deletes a frontable, freeing memory
/// @param frontable Frontable to delete
// void frontable_delete(Frontable* frontable);

/// @brief Creates a new Frontable on the heap
/// @param hash Unique hash of this frontable
/// @param name Name of frontable
/// @param pronouns Pronouns of frontable
/// @param is_custom Whether or not frontable is a custom front
/// @param color Color of frontable
/// @return A pointer to a new Frontable allocated on the heap
Frontable* frontable_create(uint32_t hash, const char* name, const char* pronouns, bool is_custom, GColor color);

uint8_t frontable_make_packed_data(bool fronting, bool is_custom, GColor color);
bool frontable_get_is_custom(const Frontable* frontable);
bool frontable_get_is_fronting(const Frontable* frontable);
void frontable_set_is_fronting(Frontable* frontable, bool fronting);
GColor frontable_get_color(const Frontable* frontable);
