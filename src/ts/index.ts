import config from "./config.json";
import * as pluralApi from "./pluralApi";
import * as pluralSocket from "./pluralSocket";
import * as cache from "./cache";

// i gotta use node CommonJS requires unfortunately, it's not a TS module
const Clay = require("pebble-clay");

// create clay config
const clay = new Clay(config);

async function setupApi(token: string) {
    console.log("setting up API in index...");

    pluralApi.init(token);
    pluralSocket.init(token);

    const id = await pluralApi.getSystemId();
    console.log(`system id ${id} fetched! caching now...`);
    cache.cacheSystemId(id);

    console.log("api set up!!");
}

Pebble.addEventListener("ready", async (e) => {
    console.log("hello !!!")

    const token = cache.getApiToken();
    if (token) {
        await setupApi(token);
    } else {
        console.warn("Warning: API Token not cached!");
    }

    // const uid = cache.getSystemId();
    // const frontables = cache.getAllFrontables();
});

// ignore this error, pebble kit TS doesn't support this event 
//   in the syntax linting but it works i swear
Pebble.addEventListener("webviewclosed", async (e: any) => {
    console.log("web view closed :]");

    if (e.response) {
        // update api key cache
        const settingsDict = clay.getSettings(e.response, false);
        const dictApiKey: string = settingsDict.PluralApiKey.value;
        if (dictApiKey) {
            cache.cacheApiToken(dictApiKey);
            await setupApi(dictApiKey);
        }

    } else {
        console.warn("webview response doesn't exist!");
    }
});

// TODO: somehow connect the clear cache function to a button in clay config
