#include "messaging.h"
#include <pebble.h>
#include "../data/config.h"
#include "../data/frontable_cache.h"
#include "../menus/main_menu.h"
#include "../menus/current_fronters_menu.h"
#include "../tools/string_tools.h"

#define FRONTABLES_PER_MESSAGE 32
#define CURRENT_FRONTS_PER_MESSAGE 16
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
}

static uint32_t uint32_from_byte_arr(uint8_t* start) {
    uint32_t num = 0;
    num |= (start[0] & 0xFF) << 24;
    num |= (start[1] & 0xFF) << 16;
    num |= (start[2] & 0xFF) << 8;
    num |= (start[3] & 0xFF) << 0;
    return num;
}

static void handle_api_inbox(DictionaryIterator* iter, ClaySettings* settings, bool* update_colors) {
    Tuple* api_key_valid = dict_find(iter, MESSAGE_KEY_ApiKeyValid);
    if (api_key_valid != NULL) {
        settings->api_key_valid = api_key_valid->value->int16;
    }

    static int frontable_counter = 0;
    static int total_frontables = 0;
    Tuple* num_total_frontables = dict_find(iter, MESSAGE_KEY_NumTotalFrontables);
    if (num_total_frontables != NULL) {
        total_frontables = num_total_frontables->value->int32;
        frontable_counter = 0;

        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Identified start of frontable message sequence, expected total transfer count: %d",
            total_frontables
        );

        cache_clear_frontables();
    }

    Tuple* frontable_hash = dict_find(iter, MESSAGE_KEY_FrontableHash);
    Tuple* frontable_name = dict_find(iter, MESSAGE_KEY_FrontableName);
    Tuple* frontable_color = dict_find(iter, MESSAGE_KEY_FrontableColor);
    Tuple* frontable_pronouns = dict_find(iter, MESSAGE_KEY_FrontablePronouns);
    Tuple* frontable_is_custom = dict_find(iter, MESSAGE_KEY_FrontableIsCustom);
    Tuple* frontable_batch_size = dict_find(iter, MESSAGE_KEY_NumFrontablesInBatch);

    // handle frontable byte data being sent
    if (
        frontable_hash != NULL &&
        frontable_name != NULL &&
        frontable_color != NULL &&
        frontable_pronouns != NULL &&
        frontable_is_custom != NULL &&
        frontable_batch_size != NULL
    ) {
        int32_t batch_size = frontable_batch_size->value->int32;
        uint8_t* hash_byte_arr = frontable_hash->value->data;
        uint8_t* color_byte_arr = frontable_color->value->data;
        uint8_t* is_custom_byte_arr = frontable_is_custom->value->data;
        char* names_combined = frontable_name->value->cstring;
        char* pronouns_combined = frontable_pronouns->value->cstring;

        uint16_t names_length = 0;
        char** names = string_split(names_combined, DELIMETER, &names_length);
        uint16_t pronouns_length = 0;
        char** pronouns = string_split(pronouns_combined, DELIMETER, &pronouns_length);

        for (int32_t i = 0; i < batch_size; i++) {
            uint32_t hash = uint32_from_byte_arr(hash_byte_arr + (i * sizeof(uint32_t)));
            uint8_t is_custom = is_custom_byte_arr[i];
            uint8_t color = color_byte_arr[i];

            Frontable* f = frontable_create(
                hash,
                names[i],
                pronouns[i],
                is_custom,
                (GColor) {.argb = color}
            );

            cache_add_frontable(f);

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

    if (frontable_counter >= total_frontables) {
        APP_LOG(APP_LOG_LEVEL_INFO, "All frontables recieved!");
        main_menu_mark_custom_fronts_loaded();
        main_menu_mark_members_loaded();
        main_menu_confirm_frontable_fetch();
    }

    static int current_front_counter = 0;
    static int total_current_fronters = 0;
    Tuple* num_current_fronters = dict_find(iter, MESSAGE_KEY_NumCurrentFronters);
    if (num_current_fronters != NULL) {
        total_current_fronters = num_current_fronters->value->int32;
        current_front_counter = 0;

        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "Identified start of current fronter message sequence, expected total transfer count: %d",
            total_current_fronters
        );

        cache_clear_current_fronters();
    }

    // handle current fronters byte data being sent
    Tuple* current_fronter = dict_find(iter, MESSAGE_KEY_CurrentFronter);
    Tuple* current_fronter_batch_size = dict_find(iter, MESSAGE_KEY_NumCurrentFrontersInBatch);
    if (current_fronter != NULL && current_fronter_batch_size != NULL) {
        int32_t batch_size = current_fronter_batch_size->value->int32;

        uint8_t* hash_byte_arr = current_fronter->value->data;

        for (int32_t i = 0; i < batch_size; i++) {
            uint32_t hash = uint32_from_byte_arr(hash_byte_arr + (i * sizeof(uint32_t)));
            cache_add_current_fronter(hash);

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
        APP_LOG(APP_LOG_LEVEL_INFO, "All current fronters recieved!");
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

void messaging_fetch_fronters() {
    bool_message(MESSAGE_KEY_FetchFrontersRequest, true);
}

void messaging_clear_cache() {
    bool_message(MESSAGE_KEY_ClearCacheRequest, true);
}
