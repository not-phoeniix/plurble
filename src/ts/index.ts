import * as cache from "./cache";
import * as messaging from "./messaging";
import * as sorting from "./sorting";
import * as config from "./config";
import { Member, AppMessageDesc, Frontable, Group, FrontEntry, APIImpl } from "./types";
import { version } from "../../package.json";

const clay = config.init();

async function setupApi(backend: APIImpl, token: string) {
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
async function fetchFrontables(backend: APIImpl, uid: string, useCache: boolean, groupPromise: Promise<any>): Promise<Frontable[]> {
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

async function fetchAndSendCurrentFronts(backend: APIImpl, uid: string): Promise<FrontEntry[]> {
    let currentFronters = await backend.endpoints.fetchGetCurrentFronters(uid);
    currentFronters = sorting.sortCurrentFronts(currentFronters);
    cache.cacheCurrentFronts(currentFronters);
    return currentFronters;
}

async function fetchGroups(backend: APIImpl, uid: string, useCache: boolean): Promise<Group[]> {
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

async function fetchAndSendAllData(backend: APIImpl, uid: string, useCache: boolean) {
    const groupPromise = fetchGroups(backend, uid, useCache);

    let frontables: Frontable[] = [];
    let currentFronters: FrontEntry[] = [];
    let groups: Group[] = [];

    await Promise.all([
        groupPromise.then(g => {
            groups = g;
        }),
        fetchFrontables(backend, uid, useCache, groupPromise).then(f => {
            frontables = f.filter(frontable => {
                return !((frontable as Member).archived);
            });
        }),
        fetchAndSendCurrentFronts(backend, uid).then(c => {
            currentFronters = c;
        }),
    ]);

    await messaging.sendDataBatchToWatch(frontables, currentFronters, groups);
}

// ~~~ init functions ~~~

function initVersionWithCache() {
    // check app version, clear cache across versions!
    const cachedVersion = cache.getAppVersion();
    if (!cachedVersion || cachedVersion !== version) {
        console.log(`New version "${version}" detected! Clearing all app cache...`);
        cache.clearAllCache();
        cache.cacheAppVersion(version);
        messaging.sendApiKeyIsValid(false);
    }
}

async function initApiWithCache(backend: APIImpl) {
    // try to get cached api token
    const token = cache.getApiToken();
    if (token) {
        await setupApi(backend, token);
    } else {
        console.warn("WARNING: API Token not cached! api can't be set up! running off cache...");
        messaging.sendApiKeyIsValid(false);
    }
}

function initFetchIntervalCache() {
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

    return useCache;
}

async function initSendInitialFetch(backend: APIImpl, uid: string, useCache: boolean) {
    try {
        await fetchAndSendAllData(backend, uid, useCache);
    } catch (err) {
        console.error(`ERROR: fetchAndSendAllData failed from ready event! err: "${err}"`);
        await messaging.sendErrorMessage("Unknown fetch error!");
    }
}

// ~~~ pebble callback setup ~~~

Pebble.addEventListener("ready", async (e) => {
    initVersionWithCache();

    const backend = config.getCurrentBackend();

    await initApiWithCache(backend);

    // try to get cached uid
    const uid = cache.getSystemId();
    if (!uid) {
        console.error("UID not cached! Cannot run fetching operations...");
        return;
    }

    const useCache = initFetchIntervalCache();
    await initSendInitialFetch(backend, uid, useCache);

    console.log("hey! app finished fetching and sending things! :)");
});

Pebble.addEventListener("appmessage", async (e) => {
    console.log("received app message !!! payload: " + JSON.stringify(e.payload));

    const msg: AppMessageDesc = e.payload;
    const backend = config.getCurrentBackend();

    const convertHash = (msgHash: number) => msgHash + Math.floor(0xFFFFFFFF / 2);

    let currentFronterUids = cache.getCurrentFronts()
        ?.map(e => e.frontableApiUid) || [];
    let frontersModified = false;

    // TODO: replace the three separate 
    //   "AddFrontRequest", "SetFrontRequest", and "RemoveFrontRequest" 
    //   messages keys with a single "set front request" that sets
    //   a new array of hashes as the current fronters
    //   (determined and calculated on the watch rather than on the ts)

    if (msg.AddFrontRequest) {
        // re-offset hash to ensure full unsigned range
        const hash = convertHash(msg.AddFrontRequest);

        console.log(`add front request identified! hash to add: ${hash}`);

        const frontable = cache.getFrontable(hash);
        if (frontable) {
            console.log(`Adding frontable ${frontable.name} to front...`);
            currentFronterUids.push(frontable.apiUid);
            frontersModified = true;
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
            currentFronterUids = [frontable.apiUid];
            frontersModified = true;
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

            // filter out all frontables that don't match the remove-requested UID
            currentFronterUids = currentFronterUids
                .filter(uid => uid !== frontable.apiUid);
            frontersModified = true;

        } else {
            console.error(`Cannot remove member from front! Member hash ${hash} was not cached!`);
        }
    }

    if (frontersModified) {
        console.log("Fronters modified... caching and setting new frontable list to this: ", currentFronterUids);

        const uid = cache.getSystemId();
        if (uid) {
            backend.endpoints.fetchSetCurrentFronters(uid, currentFronterUids)
                .then((entries) => {
                    if (entries !== undefined) {
                        cache.cacheCurrentFronts(entries);
                        messaging.sendCurrentFrontersToWatch(entries);
                    } else {
                        console.warn("WARNING: backend set fronters replied with undefined!!");
                    }
                });
        } else {
            console.warn("WARNING: cannot set new fronters, system ID was not cached!");
        }
    }

    if (msg.FetchDataRequest) {
        const uid = cache.getSystemId();
        if (uid) {
            cache.cachePrevFetchTime(Date.now());
            (async () => {
                try {
                    await fetchAndSendAllData(backend, uid, false);
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

    const backend = config.getCurrentBackend();

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

        // update backend cache
        const grabbedBackend: string = settingsDict.Backend.value;
        if (grabbedBackend) {
            cache.cacheBackend(grabbedBackend);
        }

        // update api key cache
        const grabbedToken: string = settingsDict.PluralApiKey.value;
        if (grabbedToken) {
            cache.clearAllCache();

            console.log(`API token "${grabbedToken.trim()}" grabbed from webviewclosed event!`);
            cache.cacheApiToken(grabbedToken.trim());

            console.log("Setting up API and socket again after grabbing new token!");
            await setupApi(backend, grabbedToken);

            const uid = cache.getSystemId();
            if (uid) {
                cache.cachePrevFetchTime(Date.now());
                try {
                    await fetchAndSendAllData(backend, uid, false);
                } catch (err) {
                    console.error(`ERROR: fetchAndSendAllData failed from webviewclosed event! err: "${err}"`);
                    await messaging.sendErrorMessage("Unknown crash/error!");
                }
            } else {
                console.error("Error, cannot fetch new API data, UID is not cached!");
            }
        }

        const msgDict = clay.getSettings(e.response, true);

        console.log(JSON.stringify(msgDict));

        PebbleTS.sendAppMessage(msgDict)
            .then(() => console.log("sent config data to pebble!"))
            .catch(err => console.error(`ERROR: failed to send config data to pebble! err --> ${err}`));

    } else {
        console.warn("WARNING: webview response doesn't exist!");
    }
});
