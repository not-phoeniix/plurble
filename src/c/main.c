#include <pebble.h>
#include "data/config.h"
#include "messaging/messaging.h"
#include "data/frontable_cache.h"
#include "menus/main_menu.h"
#include "menus/members_menu.h"
#include "menus/custom_fronts_menu.h"
#include "menus/current_fronters_menu.h"
#include "menus/setup_prompt_menu.h"
#include "menus/settings_menu.h"
#include "menus/disconnected_menu.h"

static void connection_handler(bool connected) {
    window_stack_pop_all(false);

    if (connected) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Phone reconnected, pushing main menu!");
        main_menu_push();
    } else {
        APP_LOG(APP_LOG_LEVEL_INFO, "Phone disconnected, pushing disconnected menu!");
        disconnected_menu_push();
    }
}

static void init() {
    // baha thanks for checking out the source code too <3
    APP_LOG(APP_LOG_LEVEL_INFO, "Hi friend, thank you for using plurble! I hope you are having a lovely day <3");

    messaging_init();
    settings_load();
    if (cache_persist_load()) {
        main_menu_mark_members_loaded();
        main_menu_mark_custom_fronts_loaded();
        main_menu_mark_fronters_loaded();
        members_menu_create_groups();
        members_menu_refresh_groupless_members();
    }

    connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = connection_handler
    });

    // push the appropriate menu depending on current watch connection status
    connection_handler(connection_service_peek_pebble_app_connection());

    main_menu_update_fronters_subtitle();
    main_menu_update_fetch_status(true);
}

static void deinit() {
    settings_save(false);
    cache_persist_store();

    connection_service_unsubscribe();

    members_menu_deinit();
    custom_fronts_menu_deinit();
    current_fronters_menu_deinit();
    main_menu_deinit();
    frontable_cache_deinit();
    settings_menu_deinit();
    disconnected_menu_deinit();

    setup_prompt_menu_remove();
}

int main() {
    init();
    app_event_loop();
    deinit();
}
