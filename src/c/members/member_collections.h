#pragma once

#include "member.h"

MemberList* members_get_members();
void members_set_members(char* members);
MemberList* members_get_fronters();
void members_set_fronters(char* fronters);
void member_collections_deinit();
Member* members_get_first_fronter();
MemberList* members_get_custom_fronts();
void members_set_custom_fronts(char* fronts);