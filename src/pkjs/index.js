// main index file, connects all modules together and sets up config page

const Clay = require("pebble-clay");
const clayConfig = require("./config.json");
const pluralApi = require("./plural_api.js");
const messaging = require("./messaging.js");
const cache = require("./cache.js");

// create clay config page
const clay = new Clay(clayConfig);

// once pebble js is ready, grab api cache and try to send members to watch
Pebble.addEventListener("ready", function (e) {
    if (pluralApi.setup(messaging.fetchAndSendDataToWatch, messaging.sendSavedFrontersToWatch)) {
        messaging.sendCachedDataToWatch();
    }
});

// on settings window close, grab api key, cache it, then send members again
Pebble.addEventListener("webviewclosed", function (e) {
    console.log("web view closed !!!");

    if (!e.response) {
        console.warn("webview response doesn't exist!");
        return;
    }

    // update api key cache
    var settingsDict = clay.getSettings(e.response, false);
    var dictApiKey = settingsDict.PluralApiKey.value;
    if (dictApiKey) {
        pluralApi.setApiToken(dictApiKey, messaging.fetchAndSendDataToWatch);
    }
});

Pebble.addEventListener("appmessage", function (e) {
    var dict = e.payload;

    console.log("got message: " + JSON.stringify(dict));

    function frontRequest(callback, memberName) {
        // attempt to find member via name
        var isCustom = false;
        var member = cache.getCachedMemberByName(memberName);
        if (!member) {
            member = cache.getCachedCustomFrontByName(memberName);
            if (member) {
                isCustom = true;
            }
        }

        // if member could be found, handle callback with them
        if (member) {
            callback(member.id, isCustom);
        }
    }

    if (dict.AddFrontRequest) {
        frontRequest(pluralApi.addToFront, dict.AddFrontRequest);
    }

    if (dict.SetFrontRequest) {
        frontRequest(pluralApi.setAsFront, dict.SetFrontRequest);
    }

    if (dict.RemoveFrontRequest) {
        frontRequest(pluralApi.removeFromFront, dict.RemoveFrontRequest);
    }
});
