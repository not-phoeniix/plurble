// cache file, handles the caching of data in phone localstorage

const utils = require("./utils.js");

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

function getFormattedMembers() {
    var membersStr = localStorage.getItem("cachedMembers");
    if (membersStr) {
        var json = JSON.parse(membersStr);

        var formattedMembers = [];
        for (var i = 0; i < json.length; i++) {
            formattedMembers.push(formatMember(json[i]));
        }

        return formattedMembers.join("|");
    }

    console.warn("Members were not cached, cannot get formatted members!");
    return null;
}

function getFormattedCustomFronts() {
    var customFrontsStr = localStorage.getItem("cachedCustomFronts");
    if (customFrontsStr) {
        var json = JSON.parse(customFrontsStr);

        var formattedCustomFronts = [];
        for (var i = 0; i < json.length; i++) {
            formattedCustomFronts.push(formatMember(json[i]));
        }

        return formattedCustomFronts.join("|");
    }

    console.warn("Custom fronts were not cached, cannot get formatted custom fronts!");
    return null;
}

function getFormattedFronters() {
    var currentFronters = getCurrentFronters();

    var fronterStrings = [];
    for (var i = 0; i < currentFronters.length; i++) {
        // try to get member first
        var fronter = getCachedMemberById(currentFronters[i].content.member);

        // then try to get custom front if member is null
        if (!fronter) {
            console.log("WAHHHHH trying to get custom front!");
            fronter = getCachedCustomFrontById(currentFronters[i].content.member);
        }

        if (!fronter) {
            console.warn("couldn't find inputted fronter with id " + currentFronters[i].content.member + "...");
        }

        // finally, push member if it exists
        if (fronter) {
            fronterStrings.push(fronter.content.name);
        }
    }

    // return formatted string joined by the "|" delimiter
    return fronterStrings.join("|");
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

function getCurrentFronters() {
    var frontersStr = localStorage.getItem("currentFronters");
    if (frontersStr) {
        return JSON.parse(frontersStr);
    }

    return null;
}

function setCurrentFronters(fronters) {
    localStorage.setItem("currentFronters", JSON.stringify(fronters));
}

/* NEW hashing system !!

function getMember(hash) {
    var members = null;
    var cache = localStorage.getItem("cachedMembers");
    if (cache) {
        members = JSON.stringify(cache);
    }

    return members[hash];
}

function getCustomFront(hash) {
    var fronts = null;
    var cache = localStorage.getItem("cachedCustomFronts");
    if (cache) {
        fronts = JSON.stringify(cache);
    }

    return fronts[hash];
}

function cacheMembers(spMemberJson) {
    var cache = {};
    for (var i = 0; i < spMemberJson.length; i++) {
        var member = spMemberJson[i];
        var hash = utils.genHash(member.id);
        cache[hash] = member;
    }

    localStorage.setItem("cachedMembers", JSON.stringify(cache));
}

*/

module.exports = {
    getCachedMemberByName: getCachedMemberByName,
    getCachedMemberById: getCachedMemberById,
    getCachedCustomFrontById: getCachedCustomFrontById,
    getCachedCustomFrontByName: getCachedCustomFrontByName,
    getFormattedMembers: getFormattedMembers,
    getFormattedCustomFronts: getFormattedCustomFronts,
    getFormattedFronters: getFormattedFronters,
    formatMember: formatMember,
    getCurrentFronters: getCurrentFronters,
    setCurrentFronters: setCurrentFronters
};
