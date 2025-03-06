#pragma once

#include <pebble.h>

/// @brief A struct that represents a member in a plural system
typedef struct {
    char name[64];
    char pronouns[64];
    GColor color;
    bool fronting;
    bool custom;
} Member;

/// @brief A struct representing a dynamic array of members
typedef struct {
    Member** members;
    uint16_t members_size;
    uint16_t num_stored;
} MemberList;

/// @brief Creates a member struct instance, automatically allocating memory
/// @param member_string Member string, in format "name,pronouns,colordecimal"
/// @return Pointer to member that was created on the heap
Member* member_create(char* member_string);

/// @brief Creates a member struct instance for a custom front entry, auto allocating memory
/// @param custom_front_string Custom front string, in format "name,,colordecimal"
/// @return Pointer to member that was created on the heap
Member* custom_front_create(char* custom_front_string);

/// @brief Deletes a member, freeing memory
/// @param member Member to delete
void member_delete(Member* member);

/// @brief Removes an element from a member list, doesn't free member memory
/// @param index_to_remove Index to remove at
/// @param array Array to remove from
/// @return Member that was removed, null if nothing removed
Member* member_list_remove_at(uint16_t index_to_remove, MemberList* array);

/// @brief Removes a member from a list via finding reference
/// @param to_remove Member to remove
/// @param array Array to remove from
/// @return True/false boolean whether or not successfully removed
bool member_list_remove(Member* to_remove, MemberList* array);

/// @brief Adds a member to the end of a member list
/// @param to_add Member to add
/// @param array Array to add to
void member_list_add(Member* to_add, MemberList* array);

/// @brief Clears a member list, doesn't free memory of members
/// @param array Array to clear
void member_list_clear(MemberList* array);

/// @brief Clears a member list and frees memory of all contained members
/// @param array Array to clear
void member_list_deep_clear(MemberList* array);
