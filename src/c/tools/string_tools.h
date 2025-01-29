#pragma once

#include <pebble.h>

/// @brief Creates/allocates a substring from a given string
/// @param str Input string to create substring from
/// @param start_index Starting index of substring
/// @param length Length of substring
/// @return A pointer to a new substring in memory
char* string_substr(char* str, uint16_t start_index, uint16_t length);

/// @brief Copies a string into the heap
/// @param source Source string to copy
/// @return Pointer to new string in the heap
char* string_copy(char* source);

/// @brief Splits a string into an array of strings via a delimiter
/// @param input Input string
/// @param delimiter Delimiter character to split string with
/// @param output_size Pointer to local variable to save output array length, will be overwritten
/// @return A pointer to a heap-allocated array of split strings
char** string_split(char* input, char delimiter, uint16_t* output_length);

/// @brief Frees memory of a string array stored on the heap
/// @param string_array Pointer to string array
/// @param length Length of string array
void string_array_free(char** string_array, uint16_t length);
