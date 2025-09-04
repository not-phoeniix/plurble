const socketUrl = "wss://api.apparyllis.com/v1/socket";
const xhrUrl = "https://api.apparyllis.com/v1/";

var socket;
var apiToken = "";
var uid = "";
var currentFronters = [];
var frontersTimout = -1;

// #region UTILITIES

function xhrRequest(urlExtension, type, data, callback) {
    if (!apiToken) {
        throw new Error("Cannot make an XHR with API token undefined!");
    }

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

function getCachedFrontable(localStorageName, searchFunc) {
    var cachedMemberString = localStorage.getItem(localStorageName);
    if (cachedMemberString) {
        var json = JSON.parse(cachedMemberString);
        for (var i = 0; i < json.length; i++) {
            if (searchFunc(json[i])) {
                return json[i];
            }
        }
    }

    return null;
}

function getCachedMemberById(id) {
    return getCachedFrontable("cachedMembers", function (member) {
        return member.id === id;
    });
}

function getCachedMemberByName(name) {
    return getCachedFrontable("cachedMembers", function (member) {
        return member.content.name === name;
    });
}

function getCachedCustomFrontById(id) {
    return getCachedFrontable("cachedCustomFronts", function (customFront) {
        return customFront.id === id
    });
}

function getCachedCustomFrontByName(name) {
    return getCachedFrontable("cachedCustomFronts", function (customFront) {
        return customFront.content.name === name;
    });
}

function getFormattedFronters() {
    var fronters = [];
    for (var i = 0; i < currentFronters.length; i++) {
        // try to get member first
        var fronter = getCachedMemberById(currentFronters[i].content.member);

        // then try to get custom front if member is null
        if (!fronter) {
            console.log("WAHHHHH trying to get custom front!");
            fronter = getCachedCustomFrontById(currentFronters[i].content.member);
        }

        if (!fronter) {
            console.log("couldn't find inputted fronter with id " + currentFronters[i].content.member + "...");
        }

        // finally, push member if it exists
        if (fronter) {
            fronters.push(fronter.content.name);
        }
    }

    // return formatted string joined by the "|" delimiter
    return fronters.join("|");
}

function formatMember(member) {
    var name = "";
    if (member.content.name) {
        name = member.content.name;
    }

    var pronouns = "";
    if (member.content.pronouns) {
        pronouns = member.content.pronouns;
    }

    //* apparently strtol doesn't exist on pebble 
    //*   lol so i need to use atoi using base 
    //*   10 numbers <3

    // strip leading "#" in hex, add a "0x",
    //   then do some weird fuckery to convert to
    //   a decimal string
    var color = "";
    if (member.content.color) {
        color = (member.content.color).slice(1);
        color = "0x" + color;
        color = Number(color).toString();
    }

    // assemble CSV string of each member data
    return name + "," + pronouns + "," + color;
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

        // interval ping to keep web socket open every 10 seconds
        setInterval(function () {
            console.log("pinging socket...");
            socket.send("ping");
        }, 10000);
    }
}

function handleFrontHistory(data) {
    // grab member result data, exit early if it doesn't exist
    var messageMember = data.results[0];
    if (!messageMember) return;

    // if member change is no longer live, remove them from current fronters!
    if (data.results[0].content.live === false) {
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

    // make it so that front current fronters are sent after 
    //   hearing silence from API for 300 ms 
    //   (prevents super often data transfer)
    clearTimeout(frontersTimout);
    frontersTimout = setTimeout(sendSavedFrontersToWatch, 300);
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
    if (socket) {
        socket.close();
    }

    socket = new WebSocket(socketUrl);
    socket.onopen = onOpenToken(token);
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
        console.log("Error: UID not found when fetching custom fronts!");
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
        console.log("Error: UID not found when fetching fronters!");
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
        console.log("Error: UID not found when fetching groups!");
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
    // find current fronter data based on member ID
    var currentFronter;
    for (var i = 0; i < currentFronters.length; i++) {
        if (currentFronters[i].content.member === id) {
            currentFronter = currentFronters[i];
            break;
        }
    }

    // exit if fronter cannot be found
    if (!currentFronter) return;

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

// #region DATA MANAGEMENT

function sendSavedFrontersToWatch() {
    Pebble.sendAppMessage(
        { "Fronters": getFormattedFronters() },
        function (data) {
            console.log("saved fronters sent!");
        },
        function (data, error) {
            console.log("Error in fronters sending!! error: " + error);
        }
    );
}

function fetchAndSendDataToWatch() {
    var numFetches = 3;
    var fetchesCompleted = 0;
    var membersFormatted = "";
    var frontersFormatted = "";
    var customFrontsFormatted = "";

    // this is probably the most jank way of doing asynchronous parallel 
    //   fetching that await all to be done... hope it works well <3

    function send() {
        Pebble.sendAppMessage(
            {
                "Members": membersFormatted,
                "CustomFronts": customFrontsFormatted,
                "Fronters": frontersFormatted,
                "ApiKeyValid": true
            },
            function (data) {
                console.log("fetched data sent!");
            },
            function (data, error) {
                console.log("ERROR in fetched data sending!! error: " + error);
            }
        );
    }

    function checkSend() {
        fetchesCompleted++;
        if (fetchesCompleted >= numFetches) {
            send();
        }
    }

    fetchMembers(function (membersJson) {
        var membersArr = [];
        for (var i = 0; i < membersJson.length; i++) {
            membersArr.push(formatMember(membersJson[i]));
        }

        membersFormatted = membersArr.join("|");
        checkSend();
    });

    fetchCustomFronts(function (customFrontsJson) {
        var customFrontsArr = [];
        for (var i = 0; i < customFrontsJson.length; i++) {
            customFrontsArr.push(formatMember(customFrontsJson[i]));
        }

        customFrontsFormatted = customFrontsArr.join("|");
        checkSend();
    });

    fetchFronters(function (fronters) {
        console.log(JSON.stringify(fronters));

        // remove all previous fronters when fetching new fronters
        currentFronters = [];

        for (var i = 0; i < fronters.length; i++) {
            currentFronters.push(fronters[i]);
        }

        frontersFormatted = getFormattedFronters();
        checkSend();
    });
}

function sendCachedDataToWatch() {
    var data = {};

    var cachedMembers = localStorage.getItem("cachedMembers");
    if (cachedMembers) {
        var json = JSON.parse(cachedMembers);

        var membersArr = [];
        for (var i = 0; i < json.length; i++) {
            membersArr.push(formatMember(json[i]));
        }

        data.Members = membersArr.join("|");
    }

    var cachedCustomFronts = localStorage.getItem("cachedCustomFronts");
    if (cachedCustomFronts) {
        var json = JSON.parse(cachedCustomFronts);

        var frontsArr = [];
        for (var i = 0; i < json.length; i++) {
            frontsArr.push(formatMember(json[i]));
        }

        data.CustomFronts = frontsArr.join("|");
    }

    Pebble.sendAppMessage(
        data,
        function (data) {
            console.log("cached data sent!");
        },
        function (data, error) {
            console.log("ERROR in cached data sending!! error: " + error);
        }
    )
}

// #endregion

function setup() {
    // get cached api token
    var cachedApiToken = localStorage.getItem("cachedApiToken");
    if (cachedApiToken) {
        apiToken = cachedApiToken;
    } else {
        console.log("api token not cached... cannot run plural api setup...");
        Pebble.sendAppMessage(
            {
                "ApiKeyValid": false
            },
            null,
            null
        );
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

    // send cached data to watch immediately for snappiness
    sendCachedDataToWatch();
}

function setApiToken(token) {
    // when setting the api token re-fetch the UID as well
    apiToken = token;
    localStorage.setItem("cachedApiToken", token);
    fetchUid(fetchAndSendDataToWatch);
    openSocket(token);
}

function onAppMessage(messageDict) {
    function frontRequest(callback, memberName) {
        // attempt to find member via name
        var isCustom = false;
        var member = getCachedMemberByName(memberName);
        if (!member) {
            member = getCachedCustomFrontByName(memberName);
            if (member) {
                isCustom = true;
            }
        }

        // if member could be found, handle callback with them
        if (member) {
            callback(member.id, isCustom);
        }
    }

    if (messageDict.AddFrontRequest) {
        frontRequest(addToFront, messageDict.AddFrontRequest);
    }

    if (messageDict.RemoveFrontRequest) {
        frontRequest(removeFromFront, messageDict.RemoveFrontRequest);
    }

    if (messageDict.SetFrontRequest) {
        frontRequest(setAsFront, messageDict.SetFrontRequest);
    }
}

module.exports = {
    setup: setup,
    setApiToken: setApiToken,
    getCachedMemberByName: getCachedMemberByName,
    getCachedMemberById: getCachedMemberById,
    getCachedCustomFrontById: getCachedCustomFrontById,
    getCachedCustomFrontByName: getCachedCustomFrontByName,
    sendMembersToWatch: fetchAndSendDataToWatch,
    onAppMessage: onAppMessage
};
