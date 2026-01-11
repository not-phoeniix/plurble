#pragma once

#include <pebble.h>

/// @brief Splits a string into an array of strings via a delimiter
/// @param input Input string
/// @param delimiter Delimiter character to split string with
/// @param output_size Pointer to local variable to save output array length, will be overwritten
/// @return A pointer to a heap-allocated array of split strings
char** string_split(const char* input, char delimiter, uint16_t* output_length);

/// @brief Frees memory of a string array stored on the heap
/// @param string_array Pointer to string array
/// @param length Length of string array
void string_array_free(char** string_array, uint16_t length);

/// @brief Identifies whether two strings are the same from beginning until one or both strings terminates
/// @param a First string to compare
/// @param b Second string to compare
/// @return Whether or not strings start with the same characters
bool string_start_same(const char* a, const char* b);

/// @brief Copies a string to a potentially smaller sized destination, ensures a null terminator
/// @param dest Pointer to destination string
/// @param src Pointer to source string
/// @param size Max size of destination to copy
void string_safe_copy(char* dest, const char* src, size_t size);
