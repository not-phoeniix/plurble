#pragma once

#include "../frontables/frontable_list.h"
#include "../frontables/frontable.h"
#include "../frontables/group.h"

FrontableList* cache_get_members();
FrontableList* cache_get_custom_fronts();
FrontableList* cache_get_current_fronters();
Frontable* cache_get_first_fronter();

void cache_add_frontable(Frontable* frontable);
void cache_clear_frontables();
void cache_add_current_fronter(uint32_t frontable_hash);
void cache_clear_current_fronters();

Group* cache_get_group(uint32_t index);

uint32_t cache_make_group(GColor color, const char* name, Group* parent);
void cache_clear_groups();

void cache_persist_store();
bool cache_persist_load();
void cache_persist_delete();

void frontable_cache_deinit();
