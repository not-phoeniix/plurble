import { FrontEntrySocketMessage, SocketMessage, AuthSocketMessage } from "./types";
import * as cache from "./cache";
import * as messaging from "./messaging";

const DEV_SOCKET_URL = "wss://devapi.apparyllis.com/v1/socket";
const NORMAL_SOCKET_URL = "wss://api.apparyllis.com/v1/socket";

// how many milliseconds to wait for no messages 
//   before sending data to the watch
const SOCKET_BATCH_TIME = 200;

let socketUrl: string;
let token: string;
let socket: WebSocket;
let closeInterval: NodeJS.Timeout | undefined = undefined;
let frontHistoryBatchTimeout: NodeJS.Timeout | undefined = undefined;

export function init(apiToken: string, useDevServer: boolean) {
    console.log(`initializing plural socket with token ${apiToken}...`);
    token = apiToken;
    socketUrl = useDevServer ? DEV_SOCKET_URL : NORMAL_SOCKET_URL;
    startSocket();
}

export function startSocket() {
    if (!token) {
        throw new Error("Cannot set up socket, token is invalid!");
    }

    if (socket) {
        socket.close();
    }

    socket = new WebSocket(socketUrl);
    socket.onopen = onOpen;
    socket.onmessage = onMessage;
}

function onOpen(e: Event) {
    console.log("Plural API WebSocket opened! Sending auth payload...");
    if (token) {
        socket.send(JSON.stringify({
            op: "authenticate",
            token: token
        }));
    } else {
        throw new Error("Cannot authenticate web socket, token was never set up!");
    }

    clearInterval(closeInterval);
    closeInterval = setInterval(
        () => {
            if (socket.readyState === WebSocket.CLOSED) {
                // restart socket if it somehow closed
                console.log("web socket closed, attempting to restart it...")
                startSocket();
            } else {
                socket.send("ping");
            }
        },
        9_000
    );
}

function onMessage(e: MessageEvent) {
    if (e.data === "pong") {
        return;
    }

    if (e.data) {
        const data: SocketMessage = JSON.parse(e.data);

        switch (data.msg) {
            case "Successfully authenticated":
                const authMessage = data as AuthSocketMessage;
                cache.cacheSystemId(authMessage.resolvedToken.uid)
                messaging.sendApiKeyIsValid(true);
                console.log("Socket successfully authenticated :D");
                break;

            case "Authentication violation: Token is missing or invalid. Goodbye :)":
                console.warn("WARNING: Token is invalid! Closing plural socket <//3");
                (async () => {
                    await messaging.sendApiKeyIsValid(false);
                    await messaging.sendErrorMessage("API token invalid!");
                })();
                break;

            case "update":
                switch (data.target) {
                    case "frontHistory":
                        handleFrontHistory(data);
                        break;
                }
                break;
        }
    }
}

function handleFrontHistory(data: FrontEntrySocketMessage) {
    for (const result of data.results) {
        // "result" can also count as a FrontEntryMessage, 
        //   it just has one extra property with the type

        if (result.content.live) {
            cache.addFrontToCache(result);
        } else {
            cache.removeFrontFromCache(result);
        }
    }

    clearTimeout(frontHistoryBatchTimeout);
    frontHistoryBatchTimeout = setTimeout(
        () => {
            console.log("Socket indicates front history changed, sending updated fronts to watch...");
            const currentFronts = cache.getCurrentFronts();
            if (currentFronts) {
                console.log(`sending list of fronters size ${currentFronts.length} to watch...`);
                messaging.sendCurrentFrontersToWatch(currentFronts);
            } else {
                console.warn("WARNING: no cached current fronts were found ? not like an issue with the array being empty. like. theres no array in the first place. oh no !!");
            }
        },
        SOCKET_BATCH_TIME
    );
}
