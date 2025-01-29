// clay setup
const Clay = require("pebble-clay");
const clayConfig = require("./config.json");
const clay = new Clay(clayConfig);

// plural api setup
const pluralApi = require("./plural_api.js");

Pebble.addEventListener("ready", pluralApi.openSocket);
Pebble.addEventListener("appmessage", function (event) {
    console.log("app message event!!!!");
    console.log("payload... : " + JSON.stringify(event.payload));
});
