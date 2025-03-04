// clay setup
const Clay = require("pebble-clay");
const clayConfig = require("./config.json");
const clay = new Clay(clayConfig);

// plural api setup
const pluralApi = require("./plural_api.js");

// once pebble js is ready, grab api cache and try to send members to watch
Pebble.addEventListener("ready", function (e) {
    pluralApi.setup();
});

// on settings window close, grab api key, cache it, then send members again
Pebble.addEventListener("webviewclosed", function (e) {
    if (!e.response) {
        return;
    }

    // update api key cache
    var settingsDict = clay.getSettings(e.response, false);
    var dictApiKey = settingsDict.PluralApiKey.value;
    if (dictApiKey) {
        pluralApi.setApiToken(dictApiKey);
    }
});

Pebble.addEventListener("appmessage", function (e) {
    var dict = e.payload;

    console.log("got message: " + JSON.stringify(dict));

    pluralApi.onAppMessage(dict);
});
