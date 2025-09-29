import { Frontable, FrontableCollection, FrontEntryMessage } from "./types";
import * as utils from "./utils";

enum CacheKeys {
    Frontables = "cachedFrontables",
    ApiToken = "cachedApiToken",
    SystemId = "cachedSystemId",
    CurrentFronts = "cachedCurrentFrontMessages",
}

export function toFrontableCollection(frontables: Frontable[]): FrontableCollection {
    const collection: FrontableCollection = {};

    for (const frontable of frontables) {
        const hash = utils.genHash(frontable.id);
        collection[hash] = frontable;
    }

    return collection;
}

export function cacheFrontables(frontables: FrontableCollection) {
    localStorage.setItem(CacheKeys.Frontables, JSON.stringify(frontables));
}

export function getAllFrontables(): FrontableCollection | null {
    const cachedFrontablesStr = localStorage.getItem(CacheKeys.Frontables);
    if (cachedFrontablesStr) {
        const frontables = JSON.parse(cachedFrontablesStr) as FrontableCollection;
        return frontables;
    }

    return null;
}

export function getFrontable(hash: number): Frontable | null {
    const frontables = getAllFrontables();
    if (frontables) {
        return frontables[hash];
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

export function getCurrentFronts(): FrontEntryMessage[] | null {
    const cachedFronts = localStorage.getItem(CacheKeys.CurrentFronts);
    if (cachedFronts) {
        return JSON.parse(cachedFronts) as FrontEntryMessage[];
    }

    return null;
}

export function cacheCurrentFronts(messages: FrontEntryMessage[]) {
    localStorage.setItem(CacheKeys.CurrentFronts, JSON.stringify(messages));
}

export function addFrontToCache(message: FrontEntryMessage) {
    console.log(`adding ${message.content.member} to front cache...`);

    let currentFronts = getCurrentFronts();
    if (currentFronts) {
        // only add fronts if they don't already exist
        if (!currentFronts.find(f => f.id == message.id)) {
            currentFronts.push(message);
        }
    } else {
        currentFronts = [message];
    }

    cacheCurrentFronts(currentFronts);
}

export function removeFrontFromCache(message: FrontEntryMessage) {
    console.log(`removing ${message.content.member} from front cache...`);

    let currentFronts = getCurrentFronts();
    if (currentFronts) {
        // only remove fronts if they exist
        const idx = currentFronts.findIndex(f => f.id == message.id);
        if (idx >= 0) {
            currentFronts.splice(idx, 1);
        }
    } else {
        currentFronts = [];
    }

    cacheCurrentFronts(currentFronts);
}

export function removeFrontFromCacheViaId(frontableId: string): FrontEntryMessage | null {
    let currentFronts = getCurrentFronts();
    let message: FrontEntryMessage | null = null;
    if (currentFronts) {
        // only remove fronts if they exist
        const idx = currentFronts.findIndex(f => f.content.member == frontableId);
        if (idx >= 0) {
            [message] = currentFronts.splice(idx, 1);
        }
    } else {
        currentFronts = [];
    }

    cacheCurrentFronts(currentFronts);
    return message;
}

export function isFronting(frontable: Frontable): boolean {
    const currentFrontMessages = getCurrentFronts();
    if (currentFrontMessages) {
        // "!!" turns this into a boolean lol
        //   (it's a double boolean NOT lol)
        return !!currentFrontMessages.find(m => m.content.member === frontable.id);
    }

    return false;
}

export function clearAllCache() {
    localStorage.clear();
}
