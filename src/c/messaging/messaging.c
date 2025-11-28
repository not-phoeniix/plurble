#include "messaging.h"
#include <pebble.h>
#include "../data/config.h"
#include "../data/frontable_cache.h"
#include "../menus/main_menu.h"
#include "../menus/current_fronters_menu.h"
#include "../menus/members_menu.h"
#include "../tools/string_tools.h"

#define DELIMETER ';'

//! NOTE: add "${workspaceFolder}/build/include/" to your
//!   include paths folder to get rid of the warnings about
//!   MESSAGE_KEY_WhateverKeys being undefined !!!!

static void handle_settings_inbox(DictionaryIterator* iter, ClaySettings* settings, bool* update_colors) {
    Tuple* accent_color = dict_find(iter, MESSAGE_KEY_AccentColor);
    if (accent_color != NULL) {
        settings->accent_color = GColorFromHEX(accent_color->value->int32);
        *update_colors = true;
    }

    Tuple* background_color = dict_find(iter, MESSAGE_KEY_BackgroundColor);
    if (background_color != NULL) {
        settings->background_color = GColorFromHEX(background_color->value->int32);
        *update_colors = true;
    }

    Tuple* compact_member_list = dict_find(iter, MESSAGE_KEY_CompactMemberList);
    if (compact_member_list != NULL) {
        settings->compact_member_list = compact_member_list->value->int16;
        *update_colors = true;
    }

    Tuple* member_color_highlight = dict_find(iter, MESSAGE_KEY_MemberColorHighlight);
    if (member_color_highlight != NULL) {
        settings->member_color_highlight = PBL_IF_COLOR_ELSE(
            member_color_highlight->value->int16,
            false
        );
        *update_colors = true;
    }

    Tuple* member_color_tag = dict_find(iter, MESSAGE_KEY_MemberColorTag);
    if (member_color_tag != NULL) {
        settings->member_color_tag = PBL_IF_COLOR_ELSE(
            member_color_tag->value->int16,
            false
        );
        *update_colors = true;
    }

    Tuple* global_fronter_accent = dict_find(iter, MESSAGE_KEY_GlobalFronterAccent);
    if (global_fronter_accent != NULL) {
        settings->global_fronter_accent = PBL_IF_COLOR_ELSE(
            global_fronter_accent->value->int16,
            false
        );
        *update_colors = true;
    }

    Tuple* group_title_accent = dict_find(iter, MESSAGE_KEY_GroupTitleAccent);
    if (group_title_accent != NULL) {
        settings->group_title_accent = PBL_IF_COLOR_ELSE(
            group_title_accent->value->int16,
            false
        );
        *update_colors = true;
    }

    Tuple* hide_members_in_root = dict_find(iter, MESSAGE_KEY_HideMembersInRoot);
    if (hide_members_in_root != NULL) {
        settings->hide_members_in_root = hide_members_in_root->value->int16;
        *update_colors = true;
        members_menu_refresh_groups();
    }
}

static uint32_t uint32_from_byte_arr(uint8_t* start) {
    uint32_t num = 0;
    num |= (start[0] & 0xFF) << 24;
    num |= (start[1] & 0xFF) << 16;
    num |= (start[2] & 0xFF) << 8;
    num |= (start[3] & 0xFF) << 0;
    return num;
}

static void handle_api_frontables(DictionaryIterator* iter) {
    // using regular ints here so APP_LOG printf doesn't yell at me lol
    static int frontable_counter = 0;
    static int total_frontables = 0;
    static Frontable** frontables_to_set = NULL;

    Tuple* num_total_frontables = dict_find(iter, MESSAGE_KEY_NumTotalFrontables);
    if (num_total_frontables != NULL) {
        total_frontables = num_total_frontables->value->int32;
        frontable_counter = 0;

        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Identified start of frontable message sequence, expected total transfer count: %d",
            total_frontables
        );

        frontables_to_set = malloc(sizeof(Frontable*) * total_frontables);
    }

    Tuple* frontable_hash = dict_find(iter, MESSAGE_KEY_FrontableHash);
    Tuple* frontable_name = dict_find(iter, MESSAGE_KEY_FrontableName);
    Tuple* frontable_color = dict_find(iter, MESSAGE_KEY_FrontableColor);
    Tuple* frontable_pronouns = dict_find(iter, MESSAGE_KEY_FrontablePronouns);
    Tuple* frontable_is_custom = dict_find(iter, MESSAGE_KEY_FrontableIsCustom);
    Tuple* frontable_group_bits = dict_find(iter, MESSAGE_KEY_FrontableGroupBitField);
    Tuple* frontable_batch_size = dict_find(iter, MESSAGE_KEY_NumFrontablesInBatch);

    bool recieved_frontables = false;

    // handle frontable byte data being sent
    if (
        frontable_hash != NULL &&
        frontable_name != NULL &&
        frontable_color != NULL &&
        frontable_pronouns != NULL &&
        frontable_is_custom != NULL &&
        frontable_group_bits != NULL &&
        frontable_batch_size != NULL
    ) {
        int32_t batch_size = frontable_batch_size->value->int32;
        uint8_t* hash_byte_arr = frontable_hash->value->data;
        uint8_t* color_byte_arr = frontable_color->value->data;
        uint8_t* is_custom_byte_arr = frontable_is_custom->value->data;
        char* names_combined = frontable_name->value->cstring;
        char* pronouns_combined = frontable_pronouns->value->cstring;
        uint8_t* group_bits_byte_arr = frontable_group_bits->value->data;

        uint16_t names_length = 0;
        char** names = string_split(names_combined, DELIMETER, &names_length);
        uint16_t pronouns_length = 0;
        char** pronouns = string_split(pronouns_combined, DELIMETER, &pronouns_length);

        for (int32_t i = 0; i < batch_size; i++) {
            uint32_t hash = uint32_from_byte_arr(hash_byte_arr + (i * sizeof(uint32_t)));
            uint32_t bitfield = uint32_from_byte_arr(group_bits_byte_arr + (i * sizeof(uint32_t)));
            uint8_t is_custom = is_custom_byte_arr[i];
            uint8_t color = color_byte_arr[i];

            Frontable* f = frontable_create(
                hash,
                names[i],
                pronouns[i],
                is_custom,
                (GColor) {.argb = color}
            );
            f->group_bit_field = bitfield;

            frontables_to_set[frontable_counter] = f;
            recieved_frontables = true;

            frontable_counter++;
            APP_LOG(
                APP_LOG_LEVEL_INFO,
                "Recieved frontable '%s'! Index: %d/%d",
                f->name,
                frontable_counter,
                total_frontables
            );
        }

        string_array_free(names, names_length);
        string_array_free(pronouns, pronouns_length);
    }

    if (frontable_counter >= total_frontables && recieved_frontables) {
        APP_LOG(APP_LOG_LEVEL_INFO, "All frontables recieved!");

        cache_clear_frontables();
        for (int i = 0; i < total_frontables; i++) {
            if (frontables_to_set == NULL) {
                APP_LOG(APP_LOG_LEVEL_WARNING, "WARNING: Trying to add frontable to cache with non-zero frontable sending count but frontable array is null");
                break;
            }

            cache_add_frontable(frontables_to_set[i]);
        }
        free(frontables_to_set);
        frontables_to_set = NULL;
        total_frontables = 0;
        frontable_counter = 0;

        main_menu_mark_custom_fronts_loaded();
        main_menu_mark_members_loaded();
        main_menu_confirm_frontable_fetch();

        // iterate across members and add them to groups
        GroupCollection* groups = cache_get_groups();
        FrontableList* members = cache_get_members();
        for (uint16_t i = 0; i < members->num_stored; i++) {
            Frontable* member = members->frontables[i];

            for (uint16_t j = 0; j < groups->num_stored; j++) {
                Group* group = groups->groups[j];

                if (((member->group_bit_field >> j) & 1) != 0) {
                    frontable_list_add(member, group->frontables);
                }
            }
        }

        members_menu_refresh_groupless_members();

    } else if (!recieved_frontables) {
        APP_LOG(APP_LOG_LEVEL_INFO, "No frontables in message detected!");
    }
}

static void handle_api_current_fronts(DictionaryIterator* iter, bool* update_colors) {
    // using regular ints here so APP_LOG printf doesn't yell at me lol
    static int current_front_counter = 0;
    static int total_current_fronters = 0;
    static uint32_t* fronters_to_set = NULL;

    Tuple* num_current_fronters = dict_find(iter, MESSAGE_KEY_NumCurrentFronters);
    if (num_current_fronters != NULL) {
        total_current_fronters = num_current_fronters->value->int32;
        current_front_counter = 0;

        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Identified start of current fronter message sequence, expected total transfer count: %d",
            total_current_fronters
        );

        // cache_clear_current_fronters();
        fronters_to_set = malloc(sizeof(uint32_t) * total_current_fronters);
    }

    // handle current fronters byte data being sent
    Tuple* current_fronter = dict_find(iter, MESSAGE_KEY_CurrentFronter);
    Tuple* current_fronter_batch_size = dict_find(iter, MESSAGE_KEY_NumCurrentFrontersInBatch);
    if (current_fronter != NULL && current_fronter_batch_size != NULL) {
        int32_t batch_size = current_fronter_batch_size->value->int32;

        uint8_t* hash_byte_arr = current_fronter->value->data;

        for (int32_t i = 0; i < batch_size; i++) {
            uint32_t hash = uint32_from_byte_arr(hash_byte_arr + (i * sizeof(uint32_t)));
            fronters_to_set[current_front_counter] = hash;

            current_front_counter++;
            APP_LOG(
                APP_LOG_LEVEL_INFO,
                "Recieved current front '%lu'! Index: %d/%d",
                hash,
                current_front_counter,
                total_current_fronters
            );
        }
    }

    if (
        (current_front_counter >= total_current_fronters && total_current_fronters != 0) ||
        (num_current_fronters != NULL && total_current_fronters == 0)
    ) {
        APP_LOG(APP_LOG_LEVEL_INFO, "All current fronters recieved! Setting fronters...");

        cache_clear_current_fronters();
        for (int i = 0; i < total_current_fronters; i++) {
            if (fronters_to_set == NULL) {
                APP_LOG(APP_LOG_LEVEL_WARNING, "WARNING: Trying to add current frontable to cache with non-zero current fronters count but array is null");
                break;
            }

            cache_add_current_fronter(fronters_to_set[i]);
        }
        free(fronters_to_set);
        fronters_to_set = NULL;
        total_current_fronters = 0;
        current_front_counter = 0;

        APP_LOG(APP_LOG_LEVEL_INFO, "New fronters set!");

        main_menu_mark_fronters_loaded();

        *update_colors = true;

        Frontable* first_fronter = cache_get_first_fronter();
        if (first_fronter != NULL) {
            main_menu_set_fronters_subtitle(first_fronter->name);
            current_fronters_menu_set_is_empty(false);
        } else {
            main_menu_set_fronters_subtitle("no one is fronting");
            current_fronters_menu_set_is_empty(true);
        }
    }
}

static void handle_api_groups(DictionaryIterator* iter) {
    static uint8_t parent_index_arr[GROUP_LIST_MAX_COUNT];
    static int32_t parent_index_counter = 0;

    // using regular ints here so APP_LOG printf doesn't yell at me lol
    static int group_counter = 0;
    static int total_groups = 0;
    static Group** groups_to_set = NULL;

    Tuple* num_total_groups = dict_find(iter, MESSAGE_KEY_NumTotalGroups);
    if (num_total_groups != NULL) {
        total_groups = num_total_groups->value->int32;
        group_counter = 0;

        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Identified start of group message sequence, expected total transfer count: %d",
            total_groups
        );

        groups_to_set = malloc(sizeof(Group*) * total_groups);

        memset(parent_index_arr, 0, sizeof(uint8_t) * total_groups);
        parent_index_counter = 0;
    }

    Tuple* group_name = dict_find(iter, MESSAGE_KEY_GroupName);
    Tuple* group_color = dict_find(iter, MESSAGE_KEY_GroupColor);
    Tuple* group_batch_size = dict_find(iter, MESSAGE_KEY_NumGroupsInBatch);
    Tuple* group_parent_index = dict_find(iter, MESSAGE_KEY_GroupParentIndex);

    bool recieved_groups = false;

    if (
        group_name != NULL &&
        group_color != NULL &&
        group_parent_index != NULL &&
        group_batch_size != NULL
    ) {
        int32_t batch_size = group_batch_size->value->int32;
        uint8_t* color_byte_arr = group_color->value->data;
        uint8_t* parent_indices = group_parent_index->value->data;
        char* names_combined = group_name->value->cstring;

        uint16_t names_length = 0;
        char** names = string_split(names_combined, DELIMETER, &names_length);

        // create groups!
        for (int32_t i = 0; i < batch_size; i++) {
            Group* group = group_create(
                names[i],
                (GColor) {.argb = color_byte_arr[i]},
                NULL
            );

            groups_to_set[group_counter] = group;
            recieved_groups = true;

            group_counter++;
            APP_LOG(
                APP_LOG_LEVEL_INFO,
                "Recieved group '%s'! Index: %d/%d",
                group->name,
                group_counter,
                total_groups
            );
        }

        // copy indices into array so we can iterate later!
        memcpy(
            &parent_index_arr[parent_index_counter],
            parent_indices,
            sizeof(uint8_t) * (uint32_t)batch_size
        );
        parent_index_counter += batch_size;

        string_array_free(names, names_length);
    }

    if (group_counter >= total_groups && recieved_groups) {
        cache_clear_groups();
        for (int i = 0; i < total_groups; i++) {
            if (groups_to_set == NULL) {
                APP_LOG(APP_LOG_LEVEL_WARNING, "WARNING: Trying to add group to cache with non-zero group sending count but group array is null");
                break;
            }

            cache_add_group(groups_to_set[i]);
        }
        free(groups_to_set);
        groups_to_set = NULL;
        total_groups = 0;
        group_counter = 0;

        // re-iterate to assign group parent pointers
        GroupCollection* groups = cache_get_groups();
        for (uint16_t i = 0; i < groups->num_stored; i++) {
            // subtract 1 from index so we can know when no parent exists
            //   (while still allowing 255 other options for the uint8)
            int16_t index = (int16_t)parent_index_arr[i] - 1;
            if (index >= 0) {
                groups->groups[i]->parent = groups->groups[index];
            }
        }

        parent_index_counter = 0;

        members_menu_refresh_groups();
        APP_LOG(APP_LOG_LEVEL_INFO, "All groups recieved!");

    } else if (!recieved_groups) {
        APP_LOG(APP_LOG_LEVEL_INFO, "No groups recognized in message!");
    }
}

static void handle_api_inbox(DictionaryIterator* iter, ClaySettings* settings, bool* update_colors) {
    Tuple* api_key_valid = dict_find(iter, MESSAGE_KEY_ApiKeyValid);
    if (api_key_valid != NULL) {
        settings->api_key_valid = api_key_valid->value->int16;
    }

    handle_api_groups(iter);
    handle_api_frontables(iter);
    handle_api_current_fronts(iter, update_colors);
}

static void inbox_recieved_handler(DictionaryIterator* iter, void* context) {
    ClaySettings* settings = settings_get();

    bool should_update_menu_colors = false;

    handle_settings_inbox(iter, settings, &should_update_menu_colors);
    handle_api_inbox(iter, settings, &should_update_menu_colors);

    settings_save(should_update_menu_colors);
}

static void inbox_dropped_callback(AppMessageResult reason, void* context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

static void outbox_sent_handler(DictionaryIterator* iter, void* context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "outbox sent!");
}

static void outbox_failed_callback(DictionaryIterator* iter, AppMessageResult reason, void* context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox failed. Reason: %d", (int)reason);
}

void messaging_init() {
    app_message_register_inbox_received(inbox_recieved_handler);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_sent(outbox_sent_handler);
    app_message_register_outbox_failed(outbox_failed_callback);

    app_message_open(2048, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}

static void front_message(uint32_t frontable_hash, const uint32_t message_key) {
    DictionaryIterator* iter;

    AppMessageResult result = app_message_outbox_begin(&iter);
    if (result == APP_MSG_OK) {
        dict_write_int32(iter, message_key, (frontable_hash - (0xFFFFFFFF / 2)));

        result = app_message_outbox_send();

        if (result != APP_MSG_OK) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending front message data: %d", (int)result);
        }

    } else {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing front message outbox: %d", (int)result);
    }
}

static void bool_message(const uint32_t key, bool value) {
    DictionaryIterator* iter;

    AppMessageResult result = app_message_outbox_begin(&iter);
    if (result == APP_MSG_OK) {
        dict_write_int16(iter, key, value);

        result = app_message_outbox_send();

        if (result != APP_MSG_OK) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "Error outbox message: %d", (int)result);
        }

    } else {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing message outbox: %d", (int)result);
    }
}

void messaging_add_to_front(uint32_t frontable_hash) {
    front_message(frontable_hash, MESSAGE_KEY_AddFrontRequest);
}

void messaging_set_as_front(uint32_t frontable_hash) {
    front_message(frontable_hash, MESSAGE_KEY_SetFrontRequest);
}

void messaging_remove_from_front(uint32_t frontable_hash) {
    front_message(frontable_hash, MESSAGE_KEY_RemoveFrontRequest);
}

void messaging_fetch_data() {
    bool_message(MESSAGE_KEY_FetchDataRequest, true);
}

void messaging_clear_cache() {
    bool_message(MESSAGE_KEY_ClearCacheRequest, true);
}
