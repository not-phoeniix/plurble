#include <pebble.h>
#include "config/config.h"
#include "messaging/messaging.h"
#include "menus/main_menu.h"
#include "menus/members_menu.h"
#include "menus/members.h"

static void update_glance(
    AppGlanceReloadSession* session,
    size_t limit,
    void* context
) {
    // exit if limit is less than 1, should never happen but just in case :]
    if (limit <= 0) return;

    // cast context to message
    const char* message = context;

    // create entry
    const AppGlanceSlice entry = {
        .layout = {
            .icon = APP_GLANCE_SLICE_DEFAULT_ICON,
            .subtitle_template_string = message
        },
        .expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION
    };

    // add slice and save result, print error if result isn't a success
    const AppGlanceResult result = app_glance_add_slice(session, entry);
    if (result != APP_GLANCE_RESULT_SUCCESS) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "AppGlance error: %d", result);
    }
}

static void init() {
    app_glance_reload(update_glance, "hiiii :]");

    messaging_init();
    settings_load();

    main_menu_push();
}

static void deinit() {
    members_menu_deinit();
    main_menu_deinit();
    members_clear();
}

int main() {
    init();
    app_event_loop();
    deinit();
}
