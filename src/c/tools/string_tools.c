#include "string_tools.h"
#include <stdlib.h>

char* string_substr(char* str, uint16_t start_index, uint16_t length) {
    char* output = (char*)malloc(sizeof(char) * (length + 1));
    memcpy(output, str + start_index, sizeof(char) * length);

    output[length] = '\0';

    return output;
}

char** string_split(char* input, char delimiter, uint16_t* output_length) {
    // cannot split a string if the delimiter is the null end character
    if (delimiter == '\0') {
        return NULL;
    }

    uint16_t length = 1;
    uint16_t i = 0;

    // count length of output array by counting number of delimiters
    while (input[i] != '\0') {
        if (input[i] == delimiter) {
            length++;
        }

        i++;
    }

    // allocate memory for an array of char pointers
    char** output = (char**)malloc(sizeof(char*) * length);

    // iterate a second time, creating/saving substrings as we go
    i = 0;
    uint16_t output_index = 0;
    uint16_t substr_start_index = 0;
    uint16_t substr_len = 0;
    while (input[i] != '\0') {
        // if delimiter is detected, create a new substring and
        //   increment counter variables
        if (input[i] == delimiter) {
            output[output_index] = string_substr(input, substr_start_index, substr_len);
            substr_len = 0;
            substr_start_index = i + 1;
            output_index++;
        } else {
            // otherwise, just increase the substring length counter
            substr_len++;
        }

        // if next character in input is the end of the string,
        //   create substring (no code should run after this)
        if (input[i + 1] == '\0') {
            output[output_index] = string_substr(input, substr_start_index, substr_len);
        }

        i++;
    }

    if (output_length != NULL) {
        *output_length = length;
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
