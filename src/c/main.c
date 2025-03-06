#include <pebble.h>
#include "config/config.h"
#include "messaging/messaging.h"
#include "menus/main_menu.h"
#include "menus/members_menu.h"
#include "members/member_collections.h"

static void init() {
    messaging_init();
    settings_load();
    main_menu_push();
}

static void deinit() {
    members_menu_deinit();
    main_menu_deinit();
    member_collections_deinit();
}

int main() {
    init();
    app_event_loop();
    deinit();
}
