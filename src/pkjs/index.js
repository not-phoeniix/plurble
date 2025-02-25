// clay setup
const Clay = require("pebble-clay");
const clayConfig = require("./config.json");
const clay = new Clay(clayConfig);

// plural api setup
const pluralApi = require("./plural_api.js");

// once pebble js is ready, grab api cache and try to send members to watch
Pebble.addEventListener("ready", function (e) {
    var cachedApiToken = localStorage.getItem("cachedApiToken");
    if (cachedApiToken) {
        pluralApi.setApiToken(cachedApiToken);
    } else {
        console.log("api token not cached...");
    }

    pluralApi.sendMembersToWatch();
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
        console.log("caching plural api key... key: " + dictApiKey);
        localStorage.setItem("cachedApiToken", dictApiKey);
        pluralApi.setApiToken(dictApiKey);
    }

    // update member list upon every webview change
    pluralApi.sendMembersToWatch();
});

