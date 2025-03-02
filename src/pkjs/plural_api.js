const socketUrl = "wss://api.apparyllis.com/v1/socket";
const xhrUrl = "https://api.apparyllis.com/v1/";
var socket;
var apiToken = "";
var uid = "";
var currentFronters = [];
var frontersTimout = -1;

// #region SOCKET SETUP

function onOpenToken(token) {
    return function (e) {
        console.log("socket opened! sending auth payload...");
        socket.send(JSON.stringify({
            op: "authenticate",
            token: token
        }));

        // interval ping to keep web socket open every 10 seconds
        setInterval(function () {
            socket.send("ping");
        }, 10000);
    }
}

function handleFrontHistory(data) {
    var member = getCachedMember(data.results[0].content.member);

    // cannot handle fronting change if member cannot be found in cache
    if (!member) return;

    // if member change is no longer live, remove them from current fronters!
    if (data.results[0].content.live === false) {
        var i = currentFronters.indexOf(member.content.name);
        if (i >= 0) {
            currentFronters.splice(i, 1);
        }
    } else {
        // else, if member change is now live, add them to the fronters list!!
        currentFronters.push(member.content.name);
    }

    // make it so that front current fronters are sent after 
    //   hearing silence from API for 200 ms 
    //   (prevents super often data transfer)
    clearTimeout(frontersTimout);
    frontersTimout = setTimeout(sendSavedFrontersToWatch, 200);
}

function handleMsg(data) {
    console.log(JSON.stringify(data));

    switch (data.msg) {
        case "Successfully authenticated":
            console.log("socket authentication successful!");
            break;

        case "Authentication violation: Token is missing or invalid. Goodbye :)":
            console.log("API key missing or invalid!");
            break;

        case "update":
            switch (data.target) {
                case "frontHistory":
                    handleFrontHistory(data);
                    break;
                default:
                    console.log("update data target \"" + data.target + "\" not recognized!");
                    break;
            }
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
            console.log("Error parsing data: " + err);
        }
    }
}

function openSocket(token) {
    socket = new WebSocket(socketUrl);
    socket.onopen = onOpenToken(token);
    socket.onmessage = onMessage;
}

// #endregion

// #region XHR FETCHING

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

function fetchUid(callback) {
    xhrRequest("me", "GET", function (response) {
        var json = JSON.parse(response);
        if (json.content.uid) {
            uid = json.content.uid;
            localStorage.setItem("cachedUid", uid);

            if (callback) {
                callback();
            }
        } else {
            console.log("UID could not be fetched!");
        }
    });
}

function fetchMembers(callback) {
    // if uid couldn't be found, print an error and exit
    if (!uid) {
        console.log("Error: UID not found when fetching members!");
        return;
    }

    xhrRequest("members/" + uid, "GET", function (response) {
        var json = JSON.parse(response);
        var membersArr = [];

        // cache members json string each time it's fetched
        localStorage.setItem("cachedMembers", response);

        for (var i = 0; i < json.length; i++) {
            var name = json[i].content.name;
            var pronouns = json[i].content.pronouns;

            //* apparently strtol doesn't exist on pebble 
            //*   lol so i need to use atoi using base 
            //*   10 numbers <3

            // strip leading "#" in hex, add a "0x",
            //   then do some weird fuckery to convert to
            //   a decimal string
            var color = (json[i].content.color).slice(1);
            color = "0x" + color;
            color = Number(color).toString();

            // assemble CSV string of each member data
            var memberCsv = name + "," + pronouns + "," + color;
            membersArr.push(memberCsv);
        }

        if (callback) {
            callback(membersArr);
        }
    });
}

function fetchFronters(callback) {
    // if uid couldn't be found, print an error and exit
    if (!uid) {
        console.log("Error: UID not found when fetching members!");
        return;
    }

    xhrRequest("fronters/", "GET", function (response) {
        try {
            var json = JSON.parse(response);
            var frontersArr = [];

            // iterate across json and search for cached member via UID
            for (var i = 0; i < json.length; i++) {
                var memberId = json[i].content.member;
                var cachedMember = getCachedMember(memberId);

                // and member name to the array if it could be found
                if (cachedMember) {
                    frontersArr.push(cachedMember.content.name);
                }
            }

            // call callback after array has been assembled
            callback(frontersArr);
        } catch (err) {
            console.log("cached member retrieval failed.. error: " + err);
        }
    });
}

// #endregion

function getCachedMember(id) {
    var cachedMemberString = localStorage.getItem("cachedMembers");
    if (cachedMemberString) {
        var json = JSON.parse(cachedMemberString);
        for (var i = 0; i < json.length; i++) {
            if (json[i].id === id) {
                return json[i];
            }
        }
    }

    return null;
}

function sendSavedFrontersToWatch() {
    var frontersStr = currentFronters.join("|");
    Pebble.sendAppMessage(
        { "Fronters": frontersStr },
        function (data) {
            console.log("saved fronters sent!");
        },
        function (data, error) {
            console.log("Error in fronters sending!! error: " + error);
        }
    );
}

function fetchAndSendDataToWatch() {
    var numFetches = 2;
    var fetchesCompleted = 0;
    var membersFormatted = "";
    var frontersFormatted = "";

    // this is probably the most jank way of doing asynchronous parallel 
    //   fetching that await all to be done... hope it works well <3

    function send() {
        Pebble.sendAppMessage(
            {
                "Members": membersFormatted,
                "Fronters": frontersFormatted
            },
            function (data) {
                console.log("members sent!");
            },
            function (data, error) {
                console.log("ERROR in member sending!! error: " + error);
            }
        );
    }

    function checkSend() {
        fetchesCompleted++;
        if (fetchesCompleted >= numFetches) {
            send();
        }
    }

    // fetch and cache members first so fronters can access cache
    fetchMembers(function (members) {
        membersFormatted = members.join("|");
        checkSend();

        // fetch fronters depending on member cache
        fetchFronters(function (fronters) {
            currentFronters = fronters;
            frontersFormatted = currentFronters.join("|");
            checkSend();
        });
    });
}

function setup() {
    // get cached api token
    var cachedApiToken = localStorage.getItem("cachedApiToken");
    if (cachedApiToken) {
        apiToken = cachedApiToken;
    } else {
        console.log("api token not cached... cannot run plural api setup...");
        return;
    }

    // get cached uid
    var cachedUid = localStorage.getItem("cachedUid");
    if (cachedUid) {
        // if uid was cached, set the value and send members
        uid = cachedUid;
        fetchAndSendDataToWatch();
    } else {
        // if uid was not cached, fetch it and send members callback
        fetchUid(fetchAndSendDataToWatch);
    }

    // set up the websocket and begin message listening
    openSocket(apiToken);
}

function setApiToken(token) {
    // when setting the api token re-fetch the UID as well
    apiToken = token;
    localStorage.setItem("cachedApiToken", token);
    fetchUid(fetchAndSendDataToWatch);
}

module.exports = {
    openSocket: openSocket,
    setup: setup,
    setApiToken: setApiToken,
    sendMembersToWatch: fetchAndSendDataToWatch
};
