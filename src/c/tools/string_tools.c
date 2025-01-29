#include "string_tools.h"
#include <stdlib.h>

char* string_substr(char* str, uint16_t start_index, uint16_t length) {
    char* output = (char*)malloc(sizeof(char) * (length + 1));
    for (uint16_t i = 0; i < length; i++) {
        output[i] = str[start_index + i];
    }

    output[length] = '\0';

    return output;
}

char* string_copy(char* source) {
    // loop through string, counting until null character is reached
    uint16_t length = 0;
    while (source[length] != '\0') {
        length++;
    }

    // allocate memory for new string
    char* copy = (char*)malloc(sizeof(char) * length);

    // re-iterate, copying all characters over
    uint16_t i = 0;
    while (source[i] != '\0') {
        copy[i] = source[i];
        i++;
    }
    copy[i] = '\0';

    // give back pointer to newly allocated string
    return copy;
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
