// plural api file, handles the tracking/updating of fronters 
//   and general communication with the simplyplural api

const cache = require("./cache.js");

const socketUrl = "wss://devapi.apparyllis.com/v1/socket";
const xhrUrl = "https://devapi.apparyllis.com/v1/";

var socket;
var socketInterval;
var apiToken = "";
var uid = "";
var frontersTimout = -1;

var sendFrontersCallback;

// #region UTILITIES

function xhrRequest(urlExtension, type, data, callback) {
    if (!apiToken) {
        throw new Error("Cannot make an XHR with API token undefined!");
    }

    console.log("making " + type + " XHR to " + xhrUrl + urlExtension + "...");

    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        if (callback) {
            callback(this.responseText);
        }
    };
    xhr.open(type, xhrUrl + urlExtension);
    xhr.setRequestHeader("Authorization", apiToken);
    xhr.setRequestHeader("Content-Type", "application/json");
    xhr.send(data ? JSON.stringify(data) : null);
}

// #endregion

// #region SOCKET SETUP

function onOpenToken(token) {
    return function (e) {
        console.log("socket opened! sending auth payload...");
        socket.send(JSON.stringify({
            op: "authenticate",
            token: token
        }));

        clearInterval(socketInterval);

        // interval ping to keep web socket open every 10 seconds
        socketInterval = setInterval(function () {
            console.log("pinging socket...");
            socket.send("ping");
        }, 10000);
    }
}

function handleFrontHistory(data) {
    // grab member result data, exit early if it doesn't exist
    var messageMember = data.results[0];
    if (!messageMember) {
        console.warn("socket front history: member at index 0 doesn't exist!");
        return;
    }

    var currentFronters = cache.getCurrentFronters();

    // if member change is no longer live, remove them from current fronters!
    if (messageMember.content.live === false) {
        // get index of inputted member so it can be removed
        var index = -1;
        for (var i = 0; i < currentFronters.length; i++) {
            if (currentFronters[i].content.member === messageMember.content.member) {
                index = i;
                break;
            }
        }

        // if index was found, remove fronter at that index from the array
        if (index > -1) {
            currentFronters.splice(index, 1);
        }

    } else {
        console.log(JSON.stringify(messageMember));

        // else, if member change is now live, add them to the fronters list!!
        currentFronters.push(messageMember);
    }

    cache.setCurrentFronters(currentFronters);

    // make it so that front current fronters are sent after 
    //   hearing silence from API for 200 ms 
    //   (prevents super often data transfer)
    clearTimeout(frontersTimout);
    frontersTimout = setTimeout(sendFrontersCallback, 200);
}

function handleMsg(data) {
    switch (data.msg) {
        case "Successfully authenticated":
            console.log("socket authentication successful!");
            break;

        case "Authentication violation: Token is missing or invalid. Goodbye :)":
            console.warn("API key missing or invalid!");
            break;

        case "update":
            console.log("socket update detected!");

            switch (data.target) {
                case "frontHistory":
                    handleFrontHistory(data);
                    break;
                default:
                    console.warn("update data target \"" + data.target + "\" not recognized!");
                    break;
            }
            break;

        default:
            console.error("msg [" + data.msg + "] not recognized!");
            console.error("msg data: " + JSON.stringify(data));
            break;
    }
}

function onMessage(e) {
    try {
        const data = JSON.parse(e.data);
        handleMsg(data);
    } catch (err) {
        if (e.data != "pong") {
            console.error("Error parsing data: " + err);
        } else {
            console.log(e.data);
        }
    }
}

function openSocket() {
    if (socket) {
        socket.close();
    }

    socket = new WebSocket(socketUrl);
    socket.onopen = onOpenToken(apiToken);
    socket.onmessage = onMessage;
}

// #endregion

// #region XHR FETCHING

function fetchUid(callback) {
    xhrRequest("me", "GET", null, function (response) {
        var json = JSON.parse(response);
        if (json.content.uid) {
            uid = json.content.uid;
            localStorage.setItem("cachedUid", uid);

            if (callback) {
                callback();
            }
        } else {
            console.error("UID could not be fetched!");
        }
    });
}

function fetchMembers(callback) {
    // if uid couldn't be found, print an error and exit
    if (!uid) {
        console.error("UID not found when fetching members!");
        return;
    }

    xhrRequest("members/" + uid, "GET", null, function (response) {
        localStorage.setItem("cachedMembers", response);

        if (callback) {
            callback(JSON.parse(response));
        }
    });
}

function fetchCustomFronts(callback) {
    // if uid couldn't be found, print an error and exit
    if (!uid) {
        console.error("UID not found when fetching custom fronts!");
        return;
    }

    xhrRequest("customFronts/" + uid, "GET", null, function (response) {
        localStorage.setItem("cachedCustomFronts", response);

        if (callback) {
            callback(JSON.parse(response));
        }
    });
}

function fetchFronters(callback) {
    // if uid couldn't be found, print an error and exit
    if (!uid) {
        console.error("UID not found when fetching fronters!");
        return;
    }

    xhrRequest("fronters/", "GET", null, function (response) {
        if (callback) {
            callback(JSON.parse(response));
        }
    });
}

function fetchGroups(callback) {
    // if uid couldn't be found, print an error and exit
    if (!uid) {
        console.error("UID not found when fetching groups!");
        return;
    }

    xhrRequest("groups/" + uid, "GET", null, function (response) {
        if (callback) {
            callback(JSON.parse(response));
        }
    });
}

// #endregion

// #region XHR SENDING

function setAsFront(id, custom) {
    var currentFronters = cache.getCurrentFronters();

    // remove all current fronters
    for (var i = 0; i < currentFronters.length; i++) {
        removeFromFront(currentFronters[i].content.member);
    }

    // add new ID to front
    addToFront(id, custom);
}

function addToFront(id, custom) {
    var options = {
        customStatus: "",
        custom: custom,
        live: true,
        startTime: Date.now(),
        member: id
    };

    xhrRequest("frontHistory/", "POST", options, function (response) {
        console.log(response);
    });
}

function removeFromFront(id) {
    var currentFronters = cache.getCurrentFronters();

    // find current fronter data based on member ID
    var currentFronter;
    for (var i = 0; i < currentFronters.length; i++) {
        if (currentFronters[i].content.member === id) {
            currentFronter = currentFronters[i];
            break;
        }
    }

    // exit if fronter cannot be found
    if (!currentFronter) {
        console.error("cannot remove fronter, fronter id " + id + " could not be found!");
        return;
    }

    // assemble options to send to the API
    var options = {
        customStatus: currentFronter.content.customStatus,
        custom: currentFronter.content.custom,
        live: false,
        startTime: currentFronter.content.startTime,
        endTime: Date.now(),
        member: id
    };

    // actually send patch request
    xhrRequest(
        "frontHistory/" + currentFronter.id,
        "PATCH",
        options,
        function (response) {
            console.log(response);
        }
    );
}

// #endregion

function setup(callback, frontersCallback) {
    console.log("setting up SimplyPlural API...");

    sendFrontersCallback = frontersCallback;

    // get cached api token
    console.log("getting cached API token...");
    var cachedApiToken = localStorage.getItem("cachedApiToken");
    if (cachedApiToken) {
        console.log("API token cached! setting value...");
        apiToken = cachedApiToken;
    } else {
        console.error("api token not cached... cannot run plural api setup...");
        Pebble.sendAppMessage(
            {
                "ApiKeyValid": false
            },
            null,
            null
        );
        return false;
    }

    // get cached uid
    console.log("getting cached UID...");
    var cachedUid = localStorage.getItem("cachedUid");
    if (cachedUid) {
        console.log("UID cached! setting value...");

        // if uid was cached, set the value and send members
        uid = cachedUid;
        if (callback) {
            callback();
        }

    } else {
        console.log("UID not cached, fetching...");

        // if uid was not cached, fetch it and send members callback
        fetchUid(callback);
    }

    // set up the websocket and begin message listening
    openSocket();

    console.log("SimplyPlural API set up!");

    return true;
}

function setApiToken(token, uidFetchCallback) {
    // when setting the api token re-fetch the UID as well
    apiToken = token;
    localStorage.setItem("cachedApiToken", token);
    fetchUid(uidFetchCallback);
    openSocket();
}

module.exports = {
    setup: setup,
    setApiToken: setApiToken,
    fetchCustomFronts: fetchCustomFronts,
    fetchFronters: fetchFronters,
    fetchGroups: fetchGroups,
    fetchMembers: fetchMembers,
    setAsFront: setAsFront,
    addToFront: addToFront,
    removeFromFront: removeFromFront,
};
