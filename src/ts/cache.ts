import { Frontable, FrontEntry2, FrontEntryMessage, Group } from "./types";
import * as sorting from "./sorting";

enum CacheKeys {
    Frontables = "cachedFrontables",
    Groups = "cachedGroups",
    ApiToken = "cachedApiToken",
    SystemId = "cachedSystemId",
    CurrentFronts = "cachedCurrentFrontMessages",
    AppVersion = "cachedAppVersion",
    PrevFetchTime = "cachedPrevFetchTime",
    FetchInterval = "cachedFetchInterval",
}

export function cacheFrontables(frontables: Frontable[]) {
    localStorage.setItem(CacheKeys.Frontables, JSON.stringify(frontables));
}

export function getAllFrontables(): Frontable[] | null {
    const cachedFrontablesStr = localStorage.getItem(CacheKeys.Frontables);
    if (cachedFrontablesStr) {
        const frontables = JSON.parse(cachedFrontablesStr) as Frontable[];
        return frontables;
    }

    return null;
}

export function getFrontable(hash: number): Frontable | null {
    const frontables = getAllFrontables();
    return frontables?.find(f => f.hash === hash) ?? null;
}

export function cacheGroups(groups: Group[]) {
    localStorage.setItem(CacheKeys.Groups, JSON.stringify(groups));
}

export function getAllGroups(): Group[] | null {
    const cachedGroupsStr = localStorage.getItem(CacheKeys.Groups);
    if (cachedGroupsStr) {
        const groups = JSON.parse(cachedGroupsStr) as Group[];
        return groups;
    }

    return null;
}

export function getApiToken(): string | null {
    return localStorage.getItem(CacheKeys.ApiToken);
}

export function cacheApiToken(token: string) {
    localStorage.setItem(CacheKeys.ApiToken, token);
}

export function getSystemId(): string | null {
    return localStorage.getItem(CacheKeys.SystemId);
}

export function cacheSystemId(id: string) {
    localStorage.setItem(CacheKeys.SystemId, id);
}

export function getCurrentFronts(): FrontEntry2[] | null {
    const cachedFronts = localStorage.getItem(CacheKeys.CurrentFronts);
    if (cachedFronts) {
        return JSON.parse(cachedFronts) as FrontEntry2[];
    }

    return null;
}

export function cacheCurrentFronts(entries: FrontEntry2[]) {
    localStorage.setItem(CacheKeys.CurrentFronts, JSON.stringify(entries));
}

export function addFrontToCache(entry: FrontEntry2) {
    console.log(`adding ${entry.frontableHash} to front cache...`);

    let currentFronts = getCurrentFronts();
    if (currentFronts) {
        // only add fronts if they don't already exist
        if (!currentFronts.find(f => f.frontableHash === entry.frontableHash)) {
            currentFronts.push(entry);
        }
    } else {
        currentFronts = [entry];
    }

    currentFronts = sorting.sortCurrentFronts(currentFronts);

    cacheCurrentFronts(currentFronts);
}

export function removeFrontFromCache(entry: FrontEntry2) {
    console.log(`removing ${entry.frontableHash} from front cache...`);

    let currentFronts = getCurrentFronts();
    if (currentFronts) {
        // only remove fronts if they exist
        const idx = currentFronts.findIndex(f => f.frontableHash === entry.frontableHash);
        if (idx >= 0) {
            currentFronts.splice(idx, 1);
        }
    } else {
        currentFronts = [];
    }

    currentFronts = sorting.sortCurrentFronts(currentFronts);

    cacheCurrentFronts(currentFronts);
}

export function removeFrontFromCacheViaHash(hash: number): FrontEntry2 | null {
    console.log(`removing ${hash} from front cache...`);

    let currentFronts = getCurrentFronts();
    let message: FrontEntry2 | null = null;
    if (currentFronts) {
        // only remove fronts if they exist
        const idx = currentFronts.findIndex(f => f.frontableHash === hash);
        if (idx >= 0) {
            [message] = currentFronts.splice(idx, 1);
        }
    } else {
        currentFronts = [];
    }

    currentFronts = sorting.sortCurrentFronts(currentFronts);

    cacheCurrentFronts(currentFronts);
    return message;
}

export function isFronting(frontable: Frontable): boolean {
    const currentFrontMessages = getCurrentFronts();
    if (currentFrontMessages) {
        // "!!" turns this into a boolean lol
        //   (it's a double boolean NOT lol)
        return !!currentFrontMessages.find(m => m.frontableHash === frontable.hash);
    }

    return false;
}

export function getAppVersion(): string | null {
    return localStorage.getItem(CacheKeys.AppVersion);
}

export function cacheAppVersion(version: string) {
    localStorage.setItem(CacheKeys.AppVersion, version);
}

export function getPrevFetchTime(): number | null {
    const time = localStorage.getItem(CacheKeys.PrevFetchTime);
    if (time) {
        return Number(time);
    }

    return null;
}

export function cachePrevFetchTime(time: number) {
    localStorage.setItem(CacheKeys.PrevFetchTime, time.toString());
}

export function getFetchInterval(): number | null {
    const interval = localStorage.getItem(CacheKeys.FetchInterval);
    if (interval) {
        return Number(interval);
    }

    return null;
}

export function cacheFetchInterval(interval: number) {
    localStorage.setItem(CacheKeys.FetchInterval, interval.toString());
}

export function clearAllCache() {
    for (const key in CacheKeys) {
        localStorage.removeItem(key);
    }
}
