import { FrontEntry, FrontEntryMessage, Frontable, MemberMessage, CustomFrontMessage } from "./types";
import * as cache from "./cache";

const FETCH_URL = "https://devapi.apparyllis.com/v1/";
let token: string;

interface RouteDescription {
    route: string;
    method: "GET" | "POST" | "PATCH" | "DELETE";
    body?: any;
}

export function init(apiToken: string) {
    console.log(`initializing plural API with token ${apiToken}...`);
    token = apiToken;
}

async function pluralMessage(routeDesc: RouteDescription): Promise<any> {
    if (!token) {
        throw new Error("Cannot send a plural API message when API token hasn't been set!");
    }

    return new Promise((resolve: (xhr: XMLHttpRequest) => void, reject) => {
        const xhr = new XMLHttpRequest();

        xhr.onload = () => {
            if (xhr.status >= 200 && xhr.status < 300) {
                resolve(xhr);
            } else {
                reject(new Error(`Request failed with status ${xhr.status}!`));
            }
        };
        xhr.onerror = () => reject(new Error("Network error!"));

        xhr.open(routeDesc.method, FETCH_URL + routeDesc.route, true);
        xhr.setRequestHeader("Authorization", token);
        xhr.setRequestHeader("Content-Type", "application/json");

        xhr.send(routeDesc ? JSON.stringify(routeDesc.body) : null);

    }).then(xhr => JSON.parse(xhr.responseText));
}

export async function addToFront(frontable: Frontable): Promise<void> {
    if (!cache.isFronting(frontable)) {
        const entry: FrontEntry = {
            customStatus: "",
            custom: !("pronouns" in frontable),
            live: true,
            startTime: Date.now(),
            member: frontable.id
        };

        return pluralMessage({
            route: "frontHistory/",
            method: "POST",
            body: entry
        }).then(frontId => {
            // create new message with the id that the API 
            //   replied with, add it to the tracking array
            const message: FrontEntryMessage = {
                id: frontId,
                exists: true,
                content: entry
            };

            cache.addFrontToCache(message);

        }).catch(err => console.log(err));
    }

    return Promise.reject(`Frontable ${frontable.id} is already fronting!`);
}

export async function removeFromFront(frontable: Frontable): Promise<void> {
    return removeFromFrontViaId(frontable.id);
}

async function removeFromFrontViaId(frontableId: string): Promise<void> {
    const removedMessage = cache.removeFrontFromCacheViaId(frontableId);
    if (removedMessage) {
        const entry: FrontEntry = {
            ...removedMessage.content,
            endTime: Date.now()
        };

        return pluralMessage({
            route: `frontHistory/${removedMessage.id}`,
            method: "POST",
            body: entry
        }).catch(err => console.log(err));
    }

    return Promise.reject(`Frontable ${frontableId} is not fronting!`);
}

export async function setAsFront(frontable: Frontable): Promise<void> {
    const currentFrontMessages = cache.getCurrentFronts();
    if (currentFrontMessages) {
        // wait for all promises to finish before moving on to cache clearing
        await Promise.all(currentFrontMessages.map(
            message => removeFromFrontViaId(message.content.member)
        )).catch(err => console.log(err));
    }

    // clear cache for current fronters
    cache.cacheCurrentFronts([]);

    return addToFront(frontable);
}

export async function getAllMembers(systemId: string): Promise<MemberMessage[]> {
    return pluralMessage({
        route: `members/${systemId}/`,
        method: "GET"
    }).then(data => data as MemberMessage[]);
}

export async function getAllCustomFronts(systemId: string): Promise<CustomFrontMessage[]> {
    return pluralMessage({
        route: `customFronts/${systemId}/`,
        method: "GET"
    }).then(data => data as CustomFrontMessage[]);
}

export async function getSystemId(): Promise<string> {
    return pluralMessage({
        route: "me/",
        method: "GET"
    }).then(data => data.id as string);
}

export async function getCurrentFronts(): Promise<FrontEntryMessage[]> {
    return pluralMessage({
        route: `fronters`,
        method: "GET"
    }).then(data => data as FrontEntryMessage[]);
}
