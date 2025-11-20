import config from "./config.json";
import * as pluralApi from "./pluralApi";
import * as pluralSocket from "./pluralSocket";
import * as cache from "./cache";
import * as messaging from "./messaging";
import { Member, CustomFront, AppMessageDesc, Frontable, Group } from "./types";
import { version } from "../../package.json";

// i gotta use node CommonJS requires unfortunately, it's not a TS module
const Clay = require("pebble-clay");
const clay = new Clay(config);

// set to true to use the SimplyPlural pretesting server 
//   when debugging/testing new functionality <3
const USE_DEV_SERVER = true;

async function setupApi(token: string) {
    console.log("setting up API and socket...");

    try {
        pluralApi.init(token, USE_DEV_SERVER);
        pluralSocket.init(token, USE_DEV_SERVER);

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
    } catch (err) {
        console.log(err);
    }
}

async function fetchAndSendFrontables(uid: string, useCache: boolean) {
    // assemble cached fronters to send to watch, fetch if missing
    let frontables: Frontable[] | null = null;
    if (useCache) {
        frontables = cache.getAllFrontables();
    } else {
        // clear frontables before fetching things again
        await messaging.sendCurrentFrontersToWatch([]);
        await messaging.sendFrontablesToWatch([]);
    }

    if (!frontables) {
        if (uid) {
            console.log("Frontables not cached, fetching from API...");

            frontables = [];

            // fetch data from api
            await Promise.all([
                pluralApi.getAllMembers(uid)
                    .then(members => members.forEach(
                        m => frontables!.push(Member.create(m))
                    )),
                pluralApi.getAllCustomFronts(uid)
                    .then(customFronts => customFronts.forEach(
                        c => frontables!.push(CustomFront.create(c))
                    ))
            ]);

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
    const currentFronters = await pluralApi.getCurrentFronts();
    cache.cacheCurrentFronts(currentFronters);
    await messaging.sendCurrentFrontersToWatch(currentFronters);
}

async function fetchAndSendGroups() {
    const groups: Group[] = [
    ];

    console.log("trying to send groups fingers crossed <3");
    messaging.sendGroupsToWatch(groups);
}

Pebble.addEventListener("ready", async (e) => {
    // check app version, clear cache across versions!
    const cachedVersion = cache.getAppVersion();
    if (!cachedVersion || cachedVersion !== version) {
        console.log(`New version "${version}" detected! Clearing all app cache...`);
        cache.clearAllCache();
        cache.cacheAppVersion(version);
        messaging.sendApiKeyIsValid(false);
    }

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

    await fetchAndSendFrontables(uid, true);
    await fetchAndSendCurrentFronts();
    await fetchAndSendGroups();

    console.log("hey! app finished fetching and sending things! :)");
});

Pebble.addEventListener("appmessage", async (e) => {
    console.log("received app message !!! payload: " + JSON.stringify(e.payload));

    const msg: AppMessageDesc = e.payload;

    const convertHash = (msgHash: number) => msgHash + Math.floor(0xFFFFFFFF / 2);

    if (msg.AddFrontRequest) {
        // re-offset hash to ensure full unsigned range
        const hash = convertHash(msg.AddFrontRequest);

        console.log(`add front request identified! hash to add: ${hash}`);

        const frontable = cache.getFrontable(hash);
        if (frontable) {
            console.log(`Adding frontable ${frontable.name} to front...`);
            pluralApi.addToFront(frontable)
                .then(null, console.log);
        } else {
            console.error(`Cannot add member to front! Member hash ${hash} was not cached!`);
        }
    }

    if (msg.SetFrontRequest) {
        // re-offset hash to ensure full unsigned range
        const hash = convertHash(msg.SetFrontRequest);

        console.log(`set front request identified! hash to set: ${hash}`);

        const frontable = cache.getFrontable(hash);
        if (frontable) {
            console.log(`Setting frontable ${frontable.name} as front...`);
            pluralApi.setAsFront(frontable)
                .then(null, console.log);
        } else {
            console.error(`Cannot set member as front! Member hash ${hash} was not cached!`);
        }
    }

    if (msg.RemoveFrontRequest) {
        // re-offset hash to ensure full unsigned range
        const hash = convertHash(msg.RemoveFrontRequest);

        console.log(`remove front request identified! hash to remove: ${hash}`);

        const frontable = cache.getFrontable(hash);
        if (frontable) {
            console.log(`Removing frontable ${frontable.name} from front...`);
            pluralApi.removeFromFront(frontable)
                .then(null, console.log);
        } else {
            console.error(`Cannot remove member from front! Member hash ${hash} was not cached!`);
        }
    }

    if (msg.FetchFrontersRequest) {
        const uid = cache.getSystemId();
        if (uid) {
            fetchAndSendFrontables(uid, false)
                .then(() => fetchAndSendCurrentFronts());
        } else {
            console.error("Cannot re-fetch fronters, system ID is not cached!");
        }
    }

    if (msg.ClearCacheRequest) {
        cache.clearAllCache();
        messaging.sendApiKeyIsValid(false);
    }
});

// ignore this error, pebble kit TS doesn't support this event 
//   in the syntax linting but it works i swear
Pebble.addEventListener("webviewclosed", async (e: any) => {
    console.log("web view closed :]");

    if (e.response) {
        // update api key cache
        const settingsDict = clay.getSettings(e.response, false);
        const grabbedToken: string = settingsDict.PluralApiKey.value;
        if (grabbedToken) {
            console.log(`API token "${grabbedToken}" grabbed from webviewclosed event!`);
            cache.cacheApiToken(grabbedToken);
            messaging.sendApiKeyIsValid(true);

            console.log("Setting up API and socket again after grabbing new token!");
            await setupApi(grabbedToken);

            const uid = cache.getSystemId();
            if (uid) {
                await fetchAndSendFrontables(uid, true);
                await fetchAndSendCurrentFronts();
            } else {
                console.error(`Error, cannot fetch frontables, UID is not cached!`);
            }
        }

    } else {
        console.warn("webview response doesn't exist!");
    }
});
