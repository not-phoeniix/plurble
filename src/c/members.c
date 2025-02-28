#include "members.h"
#include <pebble.h>
#include "tools/string_tools.h"
#include "menus/main_menu.h"
#include "menus/members_menu.h"

// ~~~ dynamic array functions ~~~

static void double_size(MemberList* array) {
    if (array->members == NULL) {
        array->members_size = 1;
        array->members = malloc(sizeof(Member*));
    } else {
        array->members_size *= 2;
        array->members = realloc(array->members, sizeof(Member*) * array->members_size);

        if (array->members == NULL) {
            printf("ERROR!!! MEMBERS REALLOC FAILED!!!");
        }
    }
}

Member* member_list_remove_at(uint16_t index_to_remove, MemberList* array) {
    if (index_to_remove >= array->num_stored) {
        return NULL;
    }

    Member* to_remove = array->members[index_to_remove];

    for (uint16_t i = index_to_remove + 1; i < array->num_stored; i++) {
        array->members[i - 1] = array->members[i];
    }

    array->num_stored--;
    return to_remove;
}

bool member_list_remove(Member* to_remove, MemberList* array) {
    for (uint16_t i = 0; i < array->num_stored; i++) {
        if (array->members[i] == to_remove) {
            member_list_remove_at(i, array);
            return true;
        }
    }

    return false;
}

void member_list_add(Member* to_add, MemberList* array) {
    while (array->num_stored >= array->members_size) {
        double_size(array);
    }

    array->members[array->num_stored] = to_add;
    array->num_stored++;
}

void member_list_clear(MemberList* array) {
    if (array->members != NULL) {
        free(array->members);
        array->members = NULL;
    }
    array->members_size = 0;
    array->num_stored = 0;
}

void member_list_deep_clear(MemberList* array) {
    for (uint16_t i = 0; i < array->num_stored; i++) {
        member_delete(array->members[i]);
    }

    member_list_clear(array);
}

// ~~~ member individual creation ~~~

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
    free(member);
}
