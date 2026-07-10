import config from "./config.json";
import * as cache from "./cache";
import * as messaging from "./messaging";
import * as sorting from "./sorting";
import * as backends from "./backends";
import { Member, AppMessageDesc, Frontable, Group, APIImpl, FrontEntry2 } from "./types";
import { version } from "../../package.json";

// TODO: make backends dynamic?
let backend: APIImpl = backends.pluralKit;

// i gotta use node CommonJS requires unfortunately, it's not a TS module
const Clay = require("pebble-clay");
const clay = new Clay(config);

async function setupApi(token: string) {
    console.log("setting up API and socket...");

    backend.setToken(token);

    try {
        console.log(`using backend: [${backend.name}]...`);

        console.log("API and socket set up!");

        let uid = cache.getSystemId();
        if (!uid) {
            console.log("system ID not cached, fetching from API...");
            uid = await backend.endpoints.fetchGetUID();
            console.log(`system id ${uid} fetched! caching now...`);
            cache.cacheSystemId(uid);
        } else {
            console.log("system id cached, continuing but fetching up-to-date id asynchronously anyways...");
            backend.endpoints.fetchGetUID()
                .then(fetchedUid => {
                    uid = fetchedUid;
                    return fetchedUid;
                })
                .then(cache.cacheSystemId)
        }

        console.log("api set up!!");
    } catch (err) {
        console.log(err);
    }
}

// sends frontables to watch, depends on groups being fetched first!
async function fetchFrontables(uid: string, useCache: boolean, groupPromise: Promise<any>): Promise<Frontable[]> {
    // assemble cached fronters to send to watch, fetch if missing
    let frontables: Frontable[] | null = null;
    if (useCache) {
        frontables = cache.getAllFrontables();
    }

    if (!frontables) {
        if (uid) {
            console.log("Frontables not cached, fetching from API...");

            frontables = await backend.endpoints.fetchGetAllFrontables(uid);
            frontables = sorting.sortFrontables(frontables);
            cache.cacheFrontables(frontables);

            console.log("Frontables fetched, assembled, and cached!");

        } else {
            throw new Error("Cannot fetch members from API, UID was never cached!");
        }
    } else {
        console.log("Frontables found in cache!");
    }

    await groupPromise;
    const groups = cache.getAllGroups();
    if (!groups) {
        throw new Error("Groups could not be found from group promise, frontable fetch failed!");
    }

    return frontables;
}

async function fetchAndSendCurrentFronts(uid: string): Promise<FrontEntry2[]> {
    let currentFronters = await backend.endpoints.fetchGetCurrentFronters(uid);
    currentFronters = sorting.sortCurrentFronts(currentFronters);
    cache.cacheCurrentFronts(currentFronters);
    return currentFronters;
}

async function fetchGroups(uid: string, useCache: boolean): Promise<Group[]> {
    let groups: Group[] | null = null;

    if (useCache) {
        groups = cache.getAllGroups();
    }

    if (!groups) {
        if (uid && backend.endpoints.fetchGetGroups) {
            console.log("Groups not cached, fetching from API...");

            groups = await backend.endpoints.fetchGetGroups(uid);

            console.log("Groups fetched! Sorting...");

            const frontables = cache.getAllFrontables();
            if (frontables) {
                groups = sorting.sortGroups(groups, frontables);
                console.log("Groups sorted! Caching...");
            } else {
                console.warn("WARNING: Frontables not found in cache, groups remain unsorted!");
            }

            cache.cacheGroups(groups);

            console.log("Groups fetched, assembled, and cached!");

        } else {
            throw new Error("Cannot fetch groups from API, UID was never cached!");
        }
    } else {
        console.log("Groups found in cache!");
    }

    return groups;
}

async function fetchAndSendAllData(uid: string, useCache: boolean) {
    const groupPromise = fetchGroups(uid, useCache);

    let frontables: Frontable[] = [];
    let currentFronters: FrontEntry2[] = [];
    let groups: Group[] = [];

    await Promise.all([
        groupPromise.then(g => {
            groups = g;
        }),
        fetchFrontables(uid, useCache, groupPromise).then(f => {
            frontables = f.filter(frontable => {
                return !((frontable as Member).archived);
            });
        }),
        fetchAndSendCurrentFronts(uid).then(c => {
            currentFronters = c;
        }),
    ]);

    await messaging.sendDataBatchToWatch(frontables, currentFronters, groups);
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

    let fetchInterval = cache.getFetchInterval();
    if (!fetchInterval) {
        // (24h in MS is a fallback)
        fetchInterval = (1000 * 60 * 60) * 24;
    }

    const prevFetchTime = cache.getPrevFetchTime();
    const timeNow = Date.now();

    let useCache = true;
    if (!prevFetchTime || timeNow - prevFetchTime >= fetchInterval) {
        console.log("Time since last fetch exceeded interval, clearing and re-fetching!");
        useCache = false;
        cache.cachePrevFetchTime(timeNow);
    }

    try {
        await fetchAndSendAllData(uid, useCache);
    } catch (err) {
        console.error(`ERROR: fetchAndSendAllData failed from ready event! err: "${err}"`);
        await messaging.sendErrorMessage("Unknown fetch error!");
    }

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
            console.log(`JUST KIDDING that has yet to be implemented...`);
            // pluralApi.addToFront(frontable)
            //     .then(null, console.log);
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
            console.log(`JUST KIDDING that has yet to be implemented...`);
            // pluralApi.setAsFront(frontable)
            //     .then(null, console.log);
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
            console.log(`JUST KIDDING that has yet to be implemented...`);
            // pluralApi.removeFromFront(frontable)
            //     .then(null, console.log);
        } else {
            console.error(`Cannot remove member from front! Member hash ${hash} was not cached!`);
        }
    }

    if (msg.FetchDataRequest) {
        const uid = cache.getSystemId();
        if (uid) {
            cache.cachePrevFetchTime(Date.now());
            (async () => {
                try {
                    await fetchAndSendAllData(uid, false);
                } catch {
                    console.error("ERROR: fetchAndSendAllData failed from appmessage event!");
                    await messaging.sendErrorMessage("Unknown fetch error!");
                }
            })();
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

    // TODO: figure out more robust way to validate API keys
    messaging.sendApiKeyIsValid(true);

    if (e.response) {
        const settingsDict = clay.getSettings(e.response, false);

        // update interval value cache
        const grabbedInterval: number = settingsDict.FetchInterval.value;
        if (grabbedInterval) {
            const intervalMs = grabbedInterval * 1000 * 60 * 60;
            console.log(`Fetch interval [${grabbedInterval}h/${intervalMs}ms] grabbed from webviewclosed event!`);
            cache.cacheFetchInterval(intervalMs);
        }

        // update api key cache
        const grabbedToken: string = settingsDict.PluralApiKey.value;
        if (grabbedToken) {
            cache.clearAllCache();

            console.log(`API token "${grabbedToken.trim()}" grabbed from webviewclosed event!`);
            cache.cacheApiToken(grabbedToken.trim());

            console.log("Setting up API and socket again after grabbing new token!");
            await setupApi(grabbedToken);

            const uid = cache.getSystemId();
            if (uid) {
                cache.cachePrevFetchTime(Date.now());
                try {
                    await fetchAndSendAllData(uid, false);
                } catch (err) {
                    console.error(`ERROR: fetchAndSendAllData failed from webviewclosed event! err: "${err}"`);
                    await messaging.sendErrorMessage("Unknown crash/error!");
                }
            } else {
                console.error("Error, cannot fetch new API data, UID is not cached!");
            }
        }

    } else {
        console.warn("WARNING: webview response doesn't exist!");
    }
});
