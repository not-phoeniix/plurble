#include "string_tools.h"
#include <stdlib.h>

char** string_split(const char* input, char delimiter, uint16_t* output_length) {
    // cannot split a string if the delimiter is the null end character
    if (delimiter == '\0') {
        return NULL;
    }

    uint16_t arr_len = 1;
    uint16_t i = 0;

    // count length of output array by counting number of delimiters
    while (input[i] != '\0') {
        if (input[i] == delimiter) {
            arr_len++;
        }

        i++;
    }

    char** output = (char**)malloc(sizeof(char*) * arr_len);

    // iterate a second time, creating & copying substrings as we go
    uint16_t str_start_index = 0;
    for (i = 0; i < arr_len; i++) {
        // determine size of string to copy
        uint16_t str_len = 0;
        while (input[str_start_index + str_len] != delimiter && input[str_start_index + str_len] != '\0') {
            str_len++;
        }

        // allocate string and copy
        output[i] = (char*)malloc(sizeof(char) * (str_len + 1));
        strncpy(output[i], &input[str_start_index], str_len);
        output[i][str_len] = '\0';

        // move forward for next iteration
        //   (skip an index to avoid delimiter we just found)
        str_start_index += str_len + 1;
    }

    if (output_length != NULL) {
        *output_length = arr_len;
    }
    return output;
}

void string_array_free(char** string_array, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        if (string_array[i] != NULL) {
            free(string_array[i]);
        }
    }

    free(string_array);
}

bool string_start_same(const char* a, const char* b) {
    do {
        if (*a != *b) return false;
        if (*a == '\0' && *b == '\0') break;

        a++;
        b++;
    } while (*a != '\0' && *b != '\0');

    return true;
}

void string_safe_copy(char* dest, const char* src, size_t size) {
    strncpy(dest, src, size);
    dest[size - 1] = '\0';
}
