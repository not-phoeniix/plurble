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
async function fetchAndSendFrontables(uid: string, useCache: boolean, groupPromise: Promise<void>) {
    // assemble cached fronters to send to watch, fetch if missing
    let frontables: Frontable[] | null = null;
    if (useCache) {
        frontables = cache.getAllFrontables();
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

    await groupPromise;
    const groups = cache.getAllGroups();
    if (!groups) {
        console.error("ERROR: groups could not be found from group promise, frontable fetch failed!");
        return;
    }

    if (frontables) {
        console.log("Sending frontables to watch...");
        console.log(`Groups size?: ${groups.length}`);
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
function fetchAndSendGroups(uid: string, useCache: boolean): {
    completedPromise: Promise<void>,
    groupPromise: Promise<void>
} {
    let groups: Group[] | null = null;

    const groupPromise = new Promise<void>(async (resolve) => {
        if (useCache) {
            groups = cache.getAllGroups();
        }

        if (!groups) {
            if (uid) {
                console.log("Groups not cached, fetching from API...");

                groups = (await pluralApi.getGroups(uid))
                    .map(m => Group.create(m));

                console.log("Groups fetched! Sorting...");

                console.log(JSON.stringify(groups));

                // sort group children alphabetically
                groups.forEach(g => g.members.sort((a, b) => {
                    console.log(`a: ${a}, b: ${b}`);
                    console.log(`hashA: ${utils.genHash(a)}, hashB: ${utils.genHash(b)}`);

                    const memberA = cache.getFrontable(utils.genHash(a));
                    if (memberA) {
                        console.log(`memberA: ${memberA.name}`);
                    }

                    const memberB = cache.getFrontable(utils.genHash(b));
                    if (memberB) {
                        console.log(`memberA: ${memberB.name}`);
                    }

                    if (!memberA || !memberB) return 0;

                    if (memberA.name.toLowerCase() > memberB.name.toLowerCase()) {
                        return 1;
                    } else if (memberA.name.toLowerCase() < memberB.name.toLowerCase()) {
                        return -1;
                    }

                    return 0;
                }));

                // sort groups themselves too!
                groups.sort((a, b) => {
                    if (a.name.toLowerCase() > b.name.toLowerCase()) {
                        return 1
                    } else if (a.name.toLowerCase() < b.name.toLowerCase()) {
                        return -1;
                    }

                    return 0;
                });

                console.log("Groups sorted! Caching...");

                cache.cacheGroups(groups);

                console.log("Groups fetched, assembled, and cached!");

            } else {
                console.error("Cannot fetch groups from API, UID was never cached!");
            }
        } else {
            console.log("Groups found in cache!");
        }

        resolve();
    });

    const completedPromise = new Promise<void>(async (resolve) => {
        await groupPromise;

        if (groups) {
            console.log("Groups found! sending to watch...");
            await messaging.sendGroupsToWatch(groups);
        } else {
            console.error("ERROR: Groups not fetched/cached properly, cannot send to watch!");
        }

        resolve();
    });

    return {
        completedPromise,
        groupPromise
    };
}

async function fetchAndSendAllData(uid: string, useCache: boolean) {
    const { completedPromise, groupPromise } = fetchAndSendGroups(uid, useCache);

    await completedPromise;
    await fetchAndSendFrontables(uid, useCache, groupPromise);
    await fetchAndSendCurrentFronts();

    // const { completedPromise, groupPromise } = fetchAndSendGroups(uid, true);
    // await completedPromise;
    // await Promise.all([
    //     // completedPromise,
    //     fetchAndSendFrontables(uid, true, groupPromise),
    //     fetchAndSendCurrentFronts(),
    // ]);
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
        console.warn("WARNING: API Token not cached! api can't be set up! running off cache...");
        messaging.sendApiKeyIsValid(false);
    }

    // try to get cached uid
    const uid = cache.getSystemId();
    if (!uid) {
        console.error("UID not cached! Cannot run fetching operations...");
        return;
    }

    await fetchAndSendAllData(uid, true);

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
            fetchAndSendAllData(uid, false);
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

            console.log("Setting up API and socket again after grabbing new token!");
            await setupApi(grabbedToken);

            const uid = cache.getSystemId();
            if (uid) {
                await fetchAndSendAllData(uid, false);
            } else {
                console.error("Error, cannot fetch new API data, UID is not cached!");
            }
        }

    } else {
        console.warn("WARNING: webview response doesn't exist!");
    }
});
