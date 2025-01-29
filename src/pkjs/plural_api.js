const apiUrl = "wss://api.apparyllis.com/v1/socket";

var socket;

function onOpenToken(token) {
    return function (e) {
        console.log("socket opened! sending auth payload...");
        socket.send(JSON.stringify({
            op: "authenticate",
            token: token
        }));

        function pingFunc() {
            socket.send("ping");
        }

        setInterval(pingFunc, 10000);
    }
}

function handleMsg(data) {
    switch (data.msg) {
        case "Successfully authenticated":
            console.log("socket authentication successful!");
            break;

        case "Authentication violation: Token is missing or invalid. Goodbye :)":
            console.log("API key missing or invalid!");
            break;

        case "update":
            break;

        default:
            console.log("msg [" + data.msg + "] not recognized!");
            console.log("msg data: " + JSON.stringify(data));
            break;
    }
}

function onMessage(e) {
    try {
        const data = JSON.parse(e.data);
        handleMsg(data);
    } catch (err) {
        if (e.data != "pong") {
            console.log("Error parsing data [" + e.data + "]");
        }
    }
}

function openSocket(token) {
    socket = new WebSocket(apiUrl);

    socket.onopen = onOpenToken(token);
    socket.onmessage = onMessage;
}

module.exports = {
    openSocket: openSocket
};
