import config from "./config.json";
import * as pluralApi from "./pluralApi";
import * as pluralSocket from "./pluralSocket";
import * as cache from "./cache";
import * as messaging from "./messaging";
import * as utils from "./utils";
import { Member, CustomFront, AppMessageDesc, Frontable, Group } from "./types";
import { version } from "../../package.json";

// local TS environment variables that won't be pushed... 
//   if you're getting errors just make an "env.json" 
//   file in the ts folder <3
import env from "./env.json";

// i gotta use node CommonJS requires unfortunately, it's not a TS module
const Clay = require("pebble-clay");
const clay = new Clay(config);

async function setupApi(token: string) {
    console.log("setting up API and socket...");

    try {
        const useDevServer = ((env as any).usePretestingServer) ?? false;
        if (useDevServer) {
            console.log("Using pretesting servers!");
        } else {
            console.log("Using normal servers!");
        }

        pluralApi.init(token, useDevServer);
        pluralSocket.init(token, useDevServer);

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

// sends frontables to watch, depends on groups being fetched first!
async function fetchAndSendFrontables(uid: string, useCache: boolean) {
    // assemble cached fronters to send to watch, fetch if missing
    let frontables: Frontable[] | null = null;
    if (useCache) {
        frontables = cache.getAllFrontables();
    } else {
        // clear frontables before fetching things again
        await messaging.sendCurrentFrontersToWatch([]);
        await messaging.sendFrontablesToWatch([], []);
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

    console.log("Getting groups for frontable data...")
    let groups = cache.getAllGroups();
    if (!groups) {
        console.log("Groups not cached, fetching...")
        groups = (await pluralApi.getGroups(uid)).map(Group.create);
        cache.cacheGroups(groups);
    } else {
        console.log("Getting groups found in cache!")
    }

    if (frontables) {
        console.log("Frontables found! sending to watch...");
        await messaging.sendFrontablesToWatch(frontables, groups);
    }
}

// fetches and sends current fronts to watch, depends on frontables being fetched first!
async function fetchAndSendCurrentFronts() {
    // always fetch and send current fronters to 
    //   watch, don't rely on cache
    const currentFronters = await pluralApi.getCurrentFronts();
    cache.cacheCurrentFronts(currentFronters);
    await messaging.sendCurrentFrontersToWatch(currentFronters);
}

// fetches and sends groups to watch! no dependencies
async function fetchAndSendGroups(uid: string, useCache: boolean) {
    let groups: Group[] | null = null;
    if (useCache) {
        groups = cache.getAllGroups();
    } else {
        // clear groups before fetching things again
        await messaging.sendGroupsToWatch([]);
    }

    if (!groups) {
        if (uid) {
            console.log("Groups not cached, fetching from API...");

            groups = (await pluralApi.getGroups(uid))
                .map(m => Group.create(m));

            // sort group children alphabetically
            groups.forEach(g => g.members.sort((a, b) => {
                const memberA = cache.getFrontable(utils.genHash(a));
                const memberB = cache.getFrontable(utils.genHash(b));

                if (!memberA || !memberB) return 0;

                if (memberA.name.toLowerCase() > memberB.name.toLowerCase()) {
                    return 1;
                } else if (memberA.name.toLowerCase() < memberB.name.toLowerCase()) {
                    return -1;
                }

                return 0;
            }));

            cache.cacheGroups(groups);

            console.log("Groups fetched, assembled, and cached!");

        } else {
            console.error("Cannot fetch groups from API, UID was never cached!");
        }
    } else {
        console.log("Groups found in cache!");
    }

    if (groups) {
        console.log("Groups found! sending to watch...");
        await messaging.sendGroupsToWatch(groups);
    }
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

    await fetchAndSendGroups(uid, true);
    await fetchAndSendFrontables(uid, true);
    await fetchAndSendCurrentFronts();

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

    if (msg.FetchDataRequest) {
        const uid = cache.getSystemId();
        if (uid) {
            fetchAndSendGroups(uid, false)
                .then(() => fetchAndSendFrontables(uid, false))
                .then(() => fetchAndSendCurrentFronts());
        } else {
            console.error("Cannot re-fetch data, system ID is not cached!");
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
