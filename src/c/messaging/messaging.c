#include "messaging.h"
#include <pebble.h>
#include "../data/config.h"
#include "../data/frontable_cache.h"
#include "../menus/main_menu.h"

//! NOTE: all the MESSAGE_KEY_WhateverKey defines are added
//!   later via the compiler.... these will look like errors
//!   in vscode but there's nothing wrong with em dw <3

static void handle_settings_inbox(DictionaryIterator* iter, ClaySettings* settings) {
    Tuple* accent_color = dict_find(iter, MESSAGE_KEY_AccentColor);
    if (accent_color != NULL) {
        settings->accent_color = GColorFromHEX(accent_color->value->int32);
    }

    Tuple* background_color = dict_find(iter, MESSAGE_KEY_BackgroundColor);
    if (background_color != NULL) {
        settings->background_color = GColorFromHEX(background_color->value->int32);
    }

    Tuple* compact_member_list = dict_find(iter, MESSAGE_KEY_CompactMemberList);
    if (compact_member_list != NULL) {
        settings->compact_member_list = compact_member_list->value->int16;
    }

    Tuple* member_color_highlight = dict_find(iter, MESSAGE_KEY_MemberColorHighlight);
    if (member_color_highlight != NULL) {
        settings->member_color_highlight = PBL_IF_COLOR_ELSE(
            member_color_highlight->value->int16,
            false
        );
    }

    Tuple* member_color_tag = dict_find(iter, MESSAGE_KEY_MemberColorTag);
    if (member_color_tag != NULL) {
        settings->member_color_tag = PBL_IF_COLOR_ELSE(
            member_color_tag->value->int16,
            false
        );
    }

    Tuple* global_fronter_accent = dict_find(iter, MESSAGE_KEY_GlobalFronterAccent);
    if (global_fronter_accent != NULL) {
        settings->global_fronter_accent = PBL_IF_COLOR_ELSE(
            global_fronter_accent->value->int16,
            false
        );
    }
}

static void handle_api_inbox(DictionaryIterator* iter, ClaySettings* settings) {
    Tuple* api_key_valid = dict_find(iter, MESSAGE_KEY_ApiKeyValid);
    if (api_key_valid != NULL) {
        settings->api_key_valid = api_key_valid->value->int16;
    }

    static int frontable_counter = 0;
    static int total_frontables = 0;
    Tuple* num_total_frontables = dict_find(iter, MESSAGE_KEY_NumTotalFrontables);
    if (num_total_frontables != NULL) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Identified start of frontable message sequence, clearing cached frontables!");

        cache_clear_frontables();

        // start the counting process!!!!!!!!
        total_frontables = num_total_frontables->value->int32;
        frontable_counter = 0;
    }
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "got an app message :3");

    Tuple* frontable_hash = dict_find(iter, MESSAGE_KEY_FrontableHash);
    Tuple* frontable_name = dict_find(iter, MESSAGE_KEY_FrontableName);
    Tuple* frontable_color = dict_find(iter, MESSAGE_KEY_FrontableColor);
    Tuple* frontable_pronouns = dict_find(iter, MESSAGE_KEY_FrontablePronouns);
    Tuple* frontable_is_custom = dict_find(iter, MESSAGE_KEY_FrontableIsCustom);

    if (
        frontable_hash != NULL &&
        frontable_name != NULL &&
        frontable_color != NULL &&
        frontable_is_custom != NULL
    ) {
        APP_LOG(
            APP_LOG_LEVEL_INFO,
            "adding a new frontable ! name: %s",
            frontable_name->value->cstring
        );

        Frontable* f = frontable_create(
            frontable_hash->value->uint32,
            frontable_name->value->cstring,
            frontable_pronouns != NULL ? frontable_pronouns->value->cstring : NULL,
            frontable_is_custom->value->int16,
            GColorFromHEX(frontable_color->value->int32)
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

    if (frontable_counter >= total_frontables && total_frontables != 0) {
        APP_LOG(APP_LOG_LEVEL_INFO, "All frontables recieved!");
        main_menu_mark_custom_fronts_loaded();
        main_menu_mark_members_loaded();
    }

    static int current_front_counter = 0;
    static int total_current_fronters = 0;
    Tuple* num_current_fronters = dict_find(iter, MESSAGE_KEY_NumCurrentFronters);
    if (num_current_fronters != NULL) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Identified start of current fronter message sequence, clearing cached current fronters!");

        cache_clear_current_fronters();

        // start the counting process :]
        total_current_fronters = num_current_fronters->value->int32;
        current_front_counter = 0;
    }

    Tuple* current_fronter = dict_find(iter, MESSAGE_KEY_CurrentFronter);
    if (current_fronter != NULL) {
        cache_add_current_fronter(current_fronter->value->uint32);
    }

    if (current_front_counter >= total_current_fronters && total_current_fronters != 0) {
        if (total_current_fronters != 0) {
            APP_LOG(APP_LOG_LEVEL_INFO, "All current fronters recieved!");
        }

        main_menu_mark_fronters_loaded();
    }
}

static void inbox_recieved_handler(DictionaryIterator* iter, void* context) {
    ClaySettings* settings = settings_get();

    handle_settings_inbox(iter, settings);
    handle_api_inbox(iter, settings);

    settings_save();
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

    app_message_open(512, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}

static void front_message(uint32_t frontable_hash, const uint32_t message_key) {
    // dictionary to send!
    DictionaryIterator* iter;

    // begin outbox app message
    AppMessageResult result = app_message_outbox_begin(&iter);
    if (result == APP_MSG_OK) {
        dict_write_uint32(iter, message_key, frontable_hash);

        result = app_message_outbox_send();

        if (result != APP_MSG_OK) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending front message data: %d", (int)result);
        }

    } else {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing front message outbox: %d", (int)result);
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
