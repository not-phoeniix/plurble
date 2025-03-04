#include "messaging.h"
#include <pebble.h>
#include "../config/config.h"
#include "../member_collections.h"

//! NOTE: all the MESSAGE_KEY_WhateverKey defines are added
//!   later via the compiler.... these will look like errors
//!   in vscode but there's nothing wrong with em dw <3

static void front_message(Member* member, const uint32_t message_key) {
    // dictionary to send!
    DictionaryIterator* iter;

    // begin outbox app message
    AppMessageResult result = app_message_outbox_begin(&iter);
    if (result == APP_MSG_OK) {
        // write member name as the request string data value
        dict_write_cstring(iter, message_key, member->name);

        // send outbox message itself
        result = app_message_outbox_send();

        if (result != APP_MSG_OK) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending front message data: %d", (int)result);
        }

    } else {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing front message outbox: %d", (int)result);
    }
}

static void inbox_recieved_handler(DictionaryIterator* iter, void* context) {
    ClaySettings* settings = settings_get();

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

    Tuple* members = dict_find(iter, MESSAGE_KEY_Members);
    if (members != NULL) {
        members_set_members(members->value->cstring);
    }

    Tuple* custom_fronts = dict_find(iter, MESSAGE_KEY_CustomFronts);
    if (custom_fronts != NULL) {
        members_set_custom_fronts(custom_fronts->value->cstring);
    }

    Tuple* fronters = dict_find(iter, MESSAGE_KEY_Fronters);
    if (fronters != NULL) {
        members_set_fronters(fronters->value->cstring);
    }

    settings_save();
}

static void inbox_dropped_callback(AppMessageResult reason, void* context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

static void outbox_sent_handler(DictionaryIterator* iter, void* context) {
    printf("outbox sent !!");
}

static void outbox_failed_callback(DictionaryIterator* iter, AppMessageResult reason, void* context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox failed. Reason: %d", (int)reason);
}

void messaging_init() {
    app_message_register_inbox_received(inbox_recieved_handler);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_sent(outbox_sent_handler);
    app_message_register_outbox_failed(outbox_failed_callback);

    // hoping 1024 bytes is enough, adjust if necessary later <3
    app_message_open(1024, 128);
}

void messaging_add_to_front(Member* member) {
    front_message(member, MESSAGE_KEY_AddFrontRequest);
}

void messaging_set_as_front(Member* member) {
    front_message(member, MESSAGE_KEY_SetFrontRequest);
}

void messaging_remove_from_front(Member* member) {
    front_message(member, MESSAGE_KEY_RemoveFrontRequest);
}
