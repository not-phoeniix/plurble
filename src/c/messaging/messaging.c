#include "messaging.h"
#include <pebble.h>
#include "../config/config.h"

#define NUM_KEYS 3

//! NOTE: all the MESSAGE_KEY_WhateverKey defines are added
//!   later via the compiler.... these will look like errors
//!   in vscode but there's nothing wrong with em dw <3

static void inbox_recieved_handler(DictionaryIterator* iter, void* context) {
    ClaySettings* settings = settings_get();

    bool setting_changed = false;

    Tuple* api_key = dict_find(iter, MESSAGE_KEY_PluralApiKey);
    if (api_key != NULL) {
        settings->plural_api_key = api_key->value->cstring;
        setting_changed = true;
    }

    Tuple* accent_color = dict_find(iter, MESSAGE_KEY_AccentColor);
    if (accent_color != NULL) {
        settings->accent_color = GColorFromHEX(accent_color->value->int32);
        setting_changed = true;
    }

    Tuple* members = dict_find(iter, MESSAGE_KEY_Members);
    if (members != NULL) {
        // parse data
    }

    // only save settings if any were updated
    if (setting_changed) {
        settings_save();
    }
}

static void outbox_recieved_handler(DictionaryIterator* iter, void* context) {
    printf("outbox recieved :]");
}

static void inbox_dropped_callback(AppMessageResult reason, void* context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

void messaging_init() {
    app_message_register_inbox_received(inbox_recieved_handler);
    app_message_register_outbox_sent(outbox_recieved_handler);

    uint32_t buffer_size = dict_calc_buffer_size(NUM_KEYS);
    app_message_open(buffer_size, buffer_size);
}
