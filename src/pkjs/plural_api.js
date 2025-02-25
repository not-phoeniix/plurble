const socketUrl = "wss://api.apparyllis.com/v1/socket";
const xhrUrl = "https://api.apparyllis.com/v1/";
var socket;
var apiToken = "";

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
    socket = new WebSocket(socketUrl);

    socket.onopen = onOpenToken(token);
    socket.onmessage = onMessage;
}

function xhrRequest(urlExtension, type, callback) {
    if (apiToken === "" || apiToken === undefined) {
        throw new Error("Cannot make an XHR with API token undefined!");
    }

    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        callback(this.responseText);
    };
    xhr.open(type, xhrUrl + urlExtension);
    xhr.setRequestHeader("Authorization", apiToken);
    xhr.send();
}

function fetchMembers(callback) {
    // get user id of yourself!
    xhrRequest("me", "GET", function (uidResponse) {
        var uidJson = JSON.parse(uidResponse);
        if (uidJson.content.uid) {
            // if uid could be found, send an XHR for members using the UID
            xhrRequest(
                "members/" + uidJson.content.uid,
                "GET",
                function (response) {
                    var membersJson = JSON.parse(response);
                    var membersArr = [];

                    for (var i = 0; i < membersJson.length; i++) {
                        membersArr.push(membersJson[i].content.name);
                    }

                    callback(membersArr);
                }
            );
        } else {
            console.log("UID could not be fetched!");
        }
    });
}

function setApiToken(value) {
    apiToken = value;
}

function sendMembersToWatch() {
    fetchMembers(function (members) {
        var membersFormatted = "";
        for (var i = 0; i < members.length; i++) {
            membersFormatted += members[i];
            if (i < members.length - 1) {
                membersFormatted += "|";
            }
        }

        console.log("formatted found members: " + membersFormatted);

        function onSuccess(data) {
            console.log("members sent!");
        }

        function onFailure(data, error) {
            console.log("ERROR in member sending!! error: " + error);
        }

        Pebble.sendAppMessage(
            { "Members": membersFormatted },
            onSuccess,
            onFailure
        );
    });
}

module.exports = {
    openSocket: openSocket,
    getMembers: fetchMembers,
    setApiToken: setApiToken,
    sendMembersToWatch: sendMembersToWatch
};
