#include "members.h"
#include <pebble.h>
#include "../tools/string_tools.h"
#include "main_menu.h"
#include "members_menu.h"

static Member** members = NULL;
static uint16_t num_members_stored = 0;
static uint16_t members_size = 0;

static void double_size() {
    if (members == NULL) {
        members_size = 1;
        members = malloc(sizeof(Member*));
    } else {
        members_size *= 2;
        members = realloc(members, sizeof(Member*) * members_size);
        if (members == NULL) {
            printf("ERROR!!! MEMBERS REALLOC FAILED!!!");
        }
    }
}

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

Member* members_add(char* member_string) {
    if (num_members_stored >= members_size) {
        double_size();
    }

    Member* new_member = member_create(member_string);
    members[num_members_stored] = new_member;
    num_members_stored++;

    return new_member;
}

void member_delete(Member* member) {
    free(member);
}

void members_clear() {
    for (uint16_t i = 0; i < num_members_stored; i++) {
        member_delete(members[i]);
    }

    free(members);
    members = NULL;
    members_size = 0;
    num_members_stored = 0;
}

void members_set_members(char* p_members) {
    if (members != NULL) {
        members_clear();
    }

    // split array by delimiter
    char** member_split = string_split(p_members, '|', &num_members_stored);

    // allocate memory for array of member pointers !
    //   then fill array with members
    members = malloc(sizeof(Member*) * num_members_stored);
    for (uint16_t i = 0; i < num_members_stored; i++) {
        members[i] = member_create(member_split[i]);
    }

    // free previous array split
    string_array_free(member_split, num_members_stored);

    // mark members as loaded to the main menu
    main_menu_mark_members_loaded();
    members_menu_reset_selected();
}

Member** members_get() {
    return members;
}

uint16_t members_get_num_members() {
    return num_members_stored;
}
