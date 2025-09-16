// messaging file, handles messages between phone and watch

const cache = require("./cache.js");
const pluralApi = require("./plural_api.js");

// TODO: remove cache module dependency
//   make it so data is passed in rather than grabbed automatically

function sendSavedFrontersToWatch() {
    Pebble.sendAppMessage(
        { "Fronters": cache.getFormattedFronters() },
        function (data) {
            console.log("saved fronters sent!");
        },
        function (data, error) {
            console.error("Error in fronters sending!! error: " + error);
        }
    );
}

function fetchAndSendDataToWatch() {
    var numFetches = 3;
    var fetchesCompleted = 0;
    var membersFormatted = "";
    var frontersFormatted = "";
    var customFrontsFormatted = "";

    console.log("fetching api data...");

    // this is probably the most jank way of doing asynchronous parallel 
    //   fetching that await all to be done... hope it works well <3

    function send() {
        console.log("sending fetched data to watch...");

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
                console.error("ERROR in fetched data sending!! error: " + error);
            }
        );
    }

    function checkSend() {
        fetchesCompleted++;
        if (fetchesCompleted >= numFetches) {
            send();
        }
    }

    console.log("fetching members...");

    pluralApi.fetchMembers(function (membersJson) {
        console.log("members fetched!");

        var membersArr = [];
        for (var i = 0; i < membersJson.length; i++) {
            membersArr.push(cache.formatMember(membersJson[i]));
        }

        membersFormatted = membersArr.join("|");
        checkSend();
    });

    console.log("fetching custom fronts...");

    pluralApi.fetchCustomFronts(function (customFrontsJson) {
        console.log("custom fronts fetched!");

        var customFrontsArr = [];
        for (var i = 0; i < customFrontsJson.length; i++) {
            customFrontsArr.push(cache.formatMember(customFrontsJson[i]));
        }

        customFrontsFormatted = customFrontsArr.join("|");
        checkSend();
    });

    console.log("fetching fronters...");

    pluralApi.fetchFronters(function (fronters) {
        console.log("fronters fetched!");

        // remove all previous fronters when fetching new fronters
        var currentFronters = [];

        for (var i = 0; i < fronters.length; i++) {
            currentFronters.push(fronters[i]);
        }

        cache.setCurrentFronters(currentFronters);

        frontersFormatted = cache.getFormattedFronters();
        checkSend();
    });
}

function sendCachedDataToWatch() {
    console.log("sending cached data to watch...");

    Pebble.sendAppMessage(
        {
            Members: cache.getFormattedMembers(),
            CustomFronts: cache.getFormattedCustomFronts()
        },
        function (data) {
            console.log("cached data sent!");
        },
        function (data, error) {
            console.error("ERROR in cached data sending!! error: " + error);
        }
    )
}

module.exports = {
    fetchAndSendDataToWatch: fetchAndSendDataToWatch,
    sendCachedDataToWatch: sendCachedDataToWatch,
    sendSavedFrontersToWatch: sendSavedFrontersToWatch
}
