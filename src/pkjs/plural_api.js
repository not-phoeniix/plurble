const socketUrl = "wss://api.apparyllis.com/v1/socket";
const xhrUrl = "https://api.apparyllis.com/v1/";
var socket;
var apiToken = "";
var uid = "";

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
        var json = JSON.parse(response);
        var frontersArr = [];

        var numFrontersFetched = 0;
        for (var i = 0; i < json.length; i++) {
            var memberId = json[i].content.member;
            xhrRequest("member/" + uid + "/" + memberId, "GET", function (response) {
                try {
                    var memberJson = JSON.parse(response);
                    var name = memberJson.content.name;
                    frontersArr.push(name);
                } catch (err) {
                    console.log("error in fetching fronter by id! " + err);
                }

                numFrontersFetched++;

                // when all fronters have been fetched, call the callback!
                if (numFrontersFetched >= json.length && callback) {
                    callback(frontersArr);
                }
            });
        }
    });
}

function sendDataToWatch() {
    var numFetches = 2;
    var fetchesCompleted = 0;
    var membersFormatted = "";
    var frontersFormatted = "";

    // this is the most jank way of doing asynchronous parallel 
    //   fetching that await all to be done... hope it works <3

    function send() {
        function onSuccess(data) {
            console.log("members sent!");
        }

        function onFailure(data, error) {
            console.log("ERROR in member sending!! error: " + error);
        }

        Pebble.sendAppMessage(
            {
                "Members": membersFormatted,
                "Fronters": frontersFormatted
            },
            onSuccess,
            onFailure
        );
    }

    function checkSend() {
        fetchesCompleted++;
        if (fetchesCompleted >= numFetches) {
            send();
        }
    }

    fetchMembers(function (members) {
        membersFormatted = members.join("|");
        console.log("formatted found members: " + membersFormatted);
        checkSend();
    });

    fetchFronters(function (fronters) {
        frontersFormatted = fronters.join("|");
        console.log("formatted found fronters: " + frontersFormatted);
        checkSend();
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
        sendDataToWatch();
    } else {
        // if uid was not cached, fetch it and send members callback
        fetchUid(sendDataToWatch);
    }
}

function setApiToken(token) {
    // when setting the api token re-fetch the UID as well
    apiToken = token;
    localStorage.setItem("cachedApiToken", token);
    fetchUid(sendDataToWatch);
}

module.exports = {
    openSocket: openSocket,
    setup: setup,
    setApiToken: setApiToken,
    sendMembersToWatch: sendDataToWatch
};
