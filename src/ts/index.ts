import config from "./config.json";
import * as pluralApi from "./pluralApi";
import * as cache from "./cache";

// i gotta use node CommonJS requires unfortunately, it's not a TS module
const Clay = require("pebble-clay");

// create clay config
const clay = new Clay(config);

Pebble.addEventListener("ready", async (e) => {
    console.log("Hi thanks for using my app :]");

    const token = cache.getApiToken();
    if (token) {
        pluralApi.init(token);
        pluralApi.getSystemId().then(cache.cacheSystemId);
    } else {
        console.warn("Warning: API Token not cached!");
    }

    // pluralApi.init();

    // pluralApi.getAllMembers()
});
