#pragma once

#include "../member.h"

void messaging_init();
void messaging_add_to_front(Member* member);
void messaging_set_as_front(Member* member);
void messaging_remove_from_front(Member* member);
