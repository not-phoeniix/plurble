// clay setup
const Clay = require("pebble-clay");
const clayConfig = require("./config.json");
const clay = new Clay(clayConfig);

// plural api setup
const pluralApi = require("./plural_api.js");
// Pebble.addEventListener("ready", pluralApi.openSocket);

// #region testing sending data to watch

const testData = {
    "Members": "test|this|one|dude|heyyy|hehe"
};

function onSuccess(data) {
    console.log("successful data send! data:\n" + JSON.stringify(data));
}

function onFailure(data, error) {
    console.log("ERROR in data sending!! error: " + error);
}

Pebble.addEventListener("ready", function (e) {
    Pebble.sendAppMessage(testData, onSuccess, onFailure);
});

// #endregion
