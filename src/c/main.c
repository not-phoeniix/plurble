#include <pebble.h>
#include "data/config.h"
#include "messaging/messaging.h"
#include "data/frontable_cache.h"
#include "menus/main_menu.h"
#include "menus/all_members_menu.h"
#include "menus/custom_fronts_menu.h"
#include "menus/fronters_menu.h"
#include "menus/setup_prompt_menu.h"

static void init() {
    messaging_init();
    settings_load();

    if (settings_get()->api_key_valid) {
        main_menu_push();
    } else {
        setup_prompt_menu_push();
    }
}

static void deinit() {
    all_members_menu_deinit();
    custom_fronts_menu_deinit();
    fronters_menu_deinit();
    main_menu_deinit();
    frontable_cache_deinit();
    setup_prompt_menu_remove();
}

int main() {
    init();
    app_event_loop();
    deinit();
}
