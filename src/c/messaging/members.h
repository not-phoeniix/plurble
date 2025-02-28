#pragma once

#include <pebble.h>

/// @brief A struct that represents a member in a plural system
typedef struct {
    char name[64];
    char pronouns[64];
    GColor color;
} Member;

/// @brief Creates a member struct instance, automatically allocating memory
/// @param member_string Member string, in format "name,pronouns,colordecimal"
/// @return Pointer to member that was created
Member* member_create(char* member_string);

/// @brief Creates a member struct instance on the heap and adds to internal list
/// @param member_string Member string, in format "name,pronouns,colordecimal"
/// @return Pointer to member that was created
Member* members_add(char* member_string);

/// @brief Deletes a member, freeing memory
/// @param member Member to delete
void member_delete(Member* member);

/// @brief Clears all members and frees memory in internal list
void members_clear();

/// @brief Sets all members in internal member list, overwrites old members
/// @param members Members string separated by delimeter '|'
void members_set_members(char* members);

/// @brief Gets the pointer the array of members in memory
/// @return Member double pointer to first member in memory
Member** members_get();

/// @brief Getter for the number of members stored internally
/// @return Unsigned integer number of members
uint16_t members_get_num_members();
