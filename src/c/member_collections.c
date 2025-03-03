#include "member_collections.h"
#include "tools/string_tools.h"
#include "menus/main_menu.h"

static MemberList all_members;
static MemberList fronters;

MemberList* members_get_all() {
    return &all_members;
}

void members_set_all(char* members) {
    member_list_deep_clear(&all_members);

    // split array by delimiter
    uint16_t num_members;
    char** member_split = string_split(members, '|', &num_members);

    // allocate memory for array of member pointers !
    //   then fill array with members
    for (uint16_t i = 0; i < num_members; i++) {
        Member* member = member_create(member_split[i]);
        member_list_add(member, &all_members);
    }

    // free previous array split
    string_array_free(member_split, all_members.num_stored);

    // mark members as loaded to the main menu
    main_menu_mark_members_loaded();
}

MemberList* members_get_fronters() {
    return &fronters;
}

Member* members_get_first_fronter() {
    if (fronters.num_stored <= 0) {
        return NULL;
    }

    return fronters.members[0];
}

void members_set_fronters(char* fronters_str) {
    // reset fronting status for everything and clear list
    for (uint16_t i = 0; i < fronters.num_stored; i++) {
        fronters.members[i]->fronting = false;
    }
    member_list_clear(&fronters);

    // if empty string, exit early
    if (fronters_str[0] == '\0') {
        main_menu_set_fronters_subtitle("no one is fronting");
        return;
    }

    uint16_t num_members;
    char** member_split = string_split(fronters_str, '|', &num_members);

    // for every split member string, compare across all
    //   stored members to find the correct member pointer
    for (uint16_t i = 0; i < num_members; i++) {
        Member* found_member = NULL;
        char* name1 = member_split[i];

        for (uint16_t j = 0; j < all_members.num_stored; j++) {
            char* name2 = all_members.members[j]->name;

            // if string names are the same, update
            //   found member and break
            if (strcmp(name1, name2) == 0) {
                found_member = all_members.members[j];
                break;
            }
        }

        if (found_member != NULL) {
            found_member->fronting = true;
            member_list_add(found_member, &fronters);
        }
    }

    string_array_free(member_split, num_members);

    // set main menu subtitle to name of the first fronter!
    main_menu_set_fronters_subtitle(fronters.members[0]->name);
}

void member_collections_deinit() {
    member_list_clear(&fronters);
    member_list_deep_clear(&all_members);
}
