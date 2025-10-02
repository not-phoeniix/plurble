#pragma once

#include "frontable_list.h"
#include "frontable.h"

FrontableList* cache_get_members();
FrontableList* cache_get_custom_fronts();
FrontableList* cache_get_current_fronters();
Frontable* cache_get_first_fronter();

void cache_add_frontable(const Frontable* frontable);
void cache_clear_frontables();
void cache_add_current_fronter(uint32_t frontable_hash);
void cache_clear_current_fronters();

void frontable_cache_deinit();
