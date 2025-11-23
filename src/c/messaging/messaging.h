#pragma once

#include "../frontables/frontable.h"

void messaging_init();
void messaging_add_to_front(uint32_t frontable_hash);
void messaging_set_as_front(uint32_t frontable_hash);
void messaging_remove_from_front(uint32_t frontable_hash);
void messaging_fetch_data();
void messaging_clear_cache();
