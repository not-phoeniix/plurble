#include "member_collections.h"
#include "../tools/string_tools.h"
#include "../menus/main_menu.h"

static MemberList all_members;
static MemberList custom_fronts;
static MemberList fronters;

MemberList* members_get_members() {
    return &all_members;
}

// deep clears out a member list destination and fills it with new members
//   according to an inputted formatted string (separated by pipes)
static void set_list_from_formatted(char* formatted_str, MemberList* destination, bool is_custom) {
    member_list_deep_clear(destination);

    // do not handle empty strings
    if (strcmp(formatted_str, "") == 0) {
        return;
    }

    // split array by delimiter and save
    uint16_t count;
    char** split = string_split(formatted_str, '|', &count);

    // allocate memory for array of member pointers !
    //   then fill array with members
    for (uint16_t i = 0; i < count; i++) {
        Member* member;
        if (is_custom) {
            member = custom_front_create(split[i]);
        } else {
            member = member_create(split[i]);
        }

        member_list_add(member, destination);
    }

    // free previous array split
    string_array_free(split, count);
}

void members_set_members(char* members) {
    set_list_from_formatted(members, &all_members, false);
    main_menu_mark_members_loaded();
}

void members_set_custom_fronts(char* fronts) {
    set_list_from_formatted(fronts, &custom_fronts, true);
    main_menu_mark_custom_fronts_loaded();
}

MemberList* members_get_custom_fronts() {
    return &custom_fronts;
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
    if (strcmp(fronters_str, "") == 0) {
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

        // search across ALL MEMBERS
        for (uint16_t j = 0; j < all_members.num_stored; j++) {
            char* name2 = all_members.members[j]->name;

            // if string names are the same, update
            //   found member and break
            if (strcmp(name1, name2) == 0) {
                found_member = all_members.members[j];
                break;
            }
        }

        // then search across ALL CUSTOM FRONTS (if not found)
        if (found_member == NULL) {
            for (uint16_t j = 0; j < custom_fronts.num_stored; j++) {
                char* name2 = custom_fronts.members[j]->name;

                // if string names are the same, update
                //   found member and break
                if (strcmp(name1, name2) == 0) {
                    found_member = custom_fronts.members[j];
                    break;
                }
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
    member_list_deep_clear(&custom_fronts);
}
