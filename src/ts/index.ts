import config from "./config.json";
import * as pluralApi from "./pluralApi";
import * as pluralSocket from "./pluralSocket";
import * as cache from "./cache";
import * as messaging from "./messaging";
import * as utils from "./utils";
import { Frontable, Member, CustomFront, CustomFrontMessage, MemberMessage } from "./types";

// i gotta use node CommonJS requires unfortunately, it's not a TS module
const Clay = require("pebble-clay");

// create clay config
const clay = new Clay(config);

async function setupApi(token: string) {
    console.log("setting up API and socket...");

    pluralApi.init(token);
    pluralSocket.init(token);

    console.log("API and socket set up!");

    let uid = cache.getSystemId();
    if (!uid) {
        console.log("system ID not cached, fetching from API...");
        uid = await pluralApi.getSystemId();
        console.log(`system id ${uid} fetched! caching now...`);
        cache.cacheSystemId(uid);
    } else {
        console.log("system id cached, continuing but fetching up-to-date id asynchronously anyways...");
        pluralApi.getSystemId().then(cache.cacheSystemId);
    }

    console.log("api set up!!");
}

async function fetchAndSendFrontables(uid: string) {
    // assemble cached fronters to send to watch, fetch if missing
    let frontables = cache.getAllFrontables();
    if (!frontables) {
        if (uid) {
            console.log("Frontables not cached, fetching from API...");

            let frontableArr: Frontable[] = [];

            // fetch data from api
            (await pluralApi.getAllMembers(uid))
                .forEach(m => frontableArr.push(Member.create(m)));
            (await pluralApi.getAllCustomFronts(uid))
                .forEach(c => frontableArr.push(CustomFront.create(c)));

            // assemble and convert, then cache
            frontables = utils.toFrontableCollection(frontableArr);
            cache.cacheFrontables(frontables);

            console.log("Frontables fetched, assembled, and cached!");

        } else {
            console.error("Cannot fetch members from API, UID was never cached!");
        }
    } else {
        console.log("Frontables found in cache!");
    }

    if (frontables) {
        console.log("Frontables found! sending to watch...");
        await messaging.sendFrontablesToWatch(frontables);
    }
}

async function fetchAndSendCurrentFronts() {
    // always fetch and send current fronters to 
    //   watch, don't rely on cache
    let currentFronters = await pluralApi.getCurrentFronts();
    await messaging.sendCurrentFrontersToWatch(currentFronters);
}

Pebble.addEventListener("ready", async (e) => {
    cache.clearAllCache();

    // try to get cached api token
    const token = cache.getApiToken();
    if (token) {
        await setupApi(token);
    } else {
        console.warn("Warning: API Token not cached! api can't be set up! running off cache...");
    }

    // try to get cached uid
    const uid = cache.getSystemId();
    if (!uid) {
        console.error("UID not cached! Cannot run fetching operations...");
        return;
    }

    await fetchAndSendFrontables(uid);
    await fetchAndSendCurrentFronts();

    console.log("hey! app finished fetching and sending things! :)");
});

Pebble.addEventListener("appmessage", async (e) => {
    console.log("received app message !!! payload: " + JSON.stringify(e.payload));

    const dict = e.payload;

    if (dict.AddFrontRequest) {
        const frontable = cache.getFrontable(dict.AddFrontRequest);
        if (frontable) {
            pluralApi.addToFront(frontable);
        } else {
            throw new Error(`Cannot add member to front! Member hash ${dict.AddFrontRequest} was not cached!`);
        }
    }

    if (dict.SetFrontRequest) {
        const frontable = cache.getFrontable(dict.SetFrontRequest);
        if (frontable) {
            pluralApi.setAsFront(frontable);
        } else {
            throw new Error(`Cannot set member as front! Member hash ${dict.AddFrontRequest} was not cached!`);
        }
    }

    if (dict.RemoveFrontRequest) {
        const frontable = cache.getFrontable(dict.RemoveFrontRequest);
        if (frontable) {
            pluralApi.removeFromFront(frontable);
        } else {
            throw new Error(`Cannot remove member from front! Member hash ${dict.AddFrontRequest} was not cached!`);
        }
    }
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
            messaging.sendApiKeyIsValid(true);
            await setupApi(dictApiKey);

            const uid = cache.getSystemId();
            if (uid) {
                await fetchAndSendFrontables(uid);
                await fetchAndSendCurrentFronts();
            } else {
                console.error(`Error, cannot fetch frontables, UID is not cached!`);
            }
        }

    } else {
        console.warn("webview response doesn't exist!");
    }
});

// TODO: somehow connect the clear cache function to a button in clay config
