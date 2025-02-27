#include "messaging.h"
#include <pebble.h>
#include "../config/config.h"
#include "../menus/members_menu.h"

#define NUM_KEYS 3

//! NOTE: all the MESSAGE_KEY_WhateverKey defines are added
//!   later via the compiler.... these will look like errors
//!   in vscode but there's nothing wrong with em dw <3

static void inbox_recieved_handler(DictionaryIterator* iter, void* context) {
    ClaySettings* settings = settings_get();

    bool setting_changed = false;

    Tuple* accent_color = dict_find(iter, MESSAGE_KEY_AccentColor);
    if (accent_color != NULL) {
        settings->accent_color = GColorFromHEX(accent_color->value->int32);
        setting_changed = true;
    }

    Tuple* compact_member_list = dict_find(iter, MESSAGE_KEY_CompactMemberList);
    if (compact_member_list != NULL) {
        settings->compact_member_list = compact_member_list->value->int16;
        setting_changed = true;
    }

    Tuple* member_color_highlight = dict_find(iter, MESSAGE_KEY_MemberColorHighlight);
    if (member_color_highlight != NULL) {
        settings->member_color_highlight = member_color_highlight->value->int16;
        setting_changed = true;
    }

    Tuple* members = dict_find(iter, MESSAGE_KEY_Members);
    if (members != NULL) {
        members_set_members(members->value->cstring, '|');
    }

    // only save settings if any were updated
    if (setting_changed) {
        settings_save();
    }
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
    app_message_open(1024, 1024);
}
