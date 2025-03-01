#pragma once

#include "member.h"

MemberList* members_get_all();
void members_set_all(char* members);
MemberList* members_get_fronters();
void members_set_fronters(char* fronters);
void member_collections_deinit();
