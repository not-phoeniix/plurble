#include "member.h"
#include <pebble.h>
#include "../tools/string_tools.h"

// Nikki,she/they|Bit,she/they|Nap,they/them
// string split !
// [
//    "Nikki,she/they",
//    "Bit,she/they",
//    "Nap,they/them",
// ]
Member* member_create(char* member_string) {
    // allocate memory and create a new member struct
    Member* member = malloc(sizeof(Member));
    *member = (Member) {
        .name = {'\0'},
        .pronouns = {'\0'},
        .color = GColorBlack
    };

    uint16_t length;
    char** csv_split = string_split(member_string, ',', &length);

    // copy data into newly created struct
    strcpy(member->name, csv_split[0]);
    strcpy(member->pronouns, csv_split[1]);
    int hex = atoi(csv_split[2]);
    member->color = GColorFromHEX(hex);

    string_array_free(csv_split, length);

    return member;
}

void member_delete(Member* member) {
    // should just have to free from memory hypothetically...
    free(member);
}
