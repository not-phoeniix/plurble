import { FrontEntrySocketMessage, SocketMessage, AuthSocketMessage } from "./types";
import * as cache from "./cache";
import * as utils from "./utils";

const SOCKET_URL = "wss://devapi.apparyllis.com/v1/socket";

let token: string;
let socket: WebSocket;
let interval: NodeJS.Timeout | undefined = undefined;

export function init(apiToken: string) {
    token = apiToken;
}

export function startSocket() {
    if (!token) {
        throw new Error("Cannot set up socket, token is invalid!");
    }

    if (socket) {
        socket.close();
    }

    socket = new WebSocket(SOCKET_URL);
    socket.onopen = onOpen;
    socket.onmessage = onMessage;
}

function onOpen(e: Event) {
    console.log("Plural API WebSocket opened! Sending auth payload...");
    socket.send(JSON.stringify({
        op: "authenticate",
        token: token
    }));

    clearInterval(interval);
    interval = setInterval(
        () => {
            console.log("socket ping...");
            socket.send("ping");
        },
        10_000
    );
}

function onMessage(e: MessageEvent) {
    if (e.data) {
        const data: SocketMessage = JSON.parse(e.data);

        switch (data.msg) {
            case "Successfully authenticated":
                const authMessage = data as AuthSocketMessage;
                cache.cacheSystemId(authMessage.resolvedToken.uid)
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
}
