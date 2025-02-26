#pragma once

/// @brief A struct that represents a member in a plural system
typedef struct {
    char name[64];
    char pronouns[64];
} Member;

/// @brief Creates a member struct instance, automatically allocating memory
/// @param member_string Member string, in format "name,pronouns"
/// @return Pointer to member that was created
Member* member_create(char* member_string);

/// @brief Deletes a member, freeing memory
/// @param member Member to delete
void member_delete(Member* member);
