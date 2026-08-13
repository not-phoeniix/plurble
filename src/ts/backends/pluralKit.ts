import { APIImpl, ConfigLayout, Group } from "../types";
import * as utils from "../utils";

const FETCH_URL = "https://api.pluralkit.me/v2/";
const FETCH_TEMPLATE = Object.freeze({
    auth: {
        headerName: "Authorization",
        token: ""
    }
});

// === TYPES ==============================================

interface MemberMessage {
    uuid: string;
    name: string;
    display_name: string;
    // FF00FF (no #)
    color: string;
    pronouns: string;
}

interface SystemMessage {
    uuid: string;
    name: string;
}

interface SwitchMessage {
    id: string;
    // an ISO string ??? (YYYY-MM-DDTHH:mm:ss.sssZ)
    timestamp: string;
    // either ids (5-6 char identifier) or member msgs
    members: string[] | MemberMessage[];
}

interface GroupMessage {
    uuid: string;
    name: string;
    color?: string;
}

// === SIMPLE GET STUFF ===================================

async function fetchGetUID() {
    console.log("hello freak");

    const { jsonData } = await utils.fetch<SystemMessage>({
        ...FETCH_TEMPLATE,
        url: FETCH_URL + "systems/@me",
        method: "GET"
    });

    return jsonData?.uuid ?? "";
}

async function fetchGetAllFrontables(uid: string) {
    const { jsonData } = await utils.fetch<MemberMessage[]>({
        ...FETCH_TEMPLATE,
        url: FETCH_URL + `systems/${uid}/members`,
        method: "GET"
    });

    if (!jsonData) {
        return [];
    }

    return jsonData.map(msg => ({
        name: msg.name,
        color: `#${msg.color}`,
        pronouns: msg.pronouns,
        archived: false,
        apiUid: msg.uuid,
        hash: utils.genHash(msg.uuid),
        isCustom: false
    }));
}

async function fetchGetCurrentFronters(uid: string) {
    const { jsonData } = await utils.fetch<SwitchMessage>({
        ...FETCH_TEMPLATE,
        url: FETCH_URL + `systems/${uid}/fronters`,
        method: "GET"
    });

    if (!jsonData) {
        return [];
    }

    return jsonData.members.map(member => {
        if (typeof member !== "object") {
            return undefined;
        }

        return {
            frontableApiUid: member.uuid,
            frontableHash: utils.genHash(member.uuid),
            startTime: new Date(jsonData.timestamp).getTime(),
            // endTime: 0,
        };
    }).filter(e => e !== undefined);
}

async function fetchGetGroups(uid: string) {
    const { jsonData } = await utils.fetch<GroupMessage[]>({
        ...FETCH_TEMPLATE,
        url: FETCH_URL + `systems/${uid}/groups`,
        method: "GET"
    });

    if (!jsonData) {
        return [];
    }

    let groups: Group[] = [];

    // for every found group, fetch their members 
    //   (and async await for all of them to be done in parallel)
    await Promise.all(jsonData.map(async (groupMsg) => {
        const { jsonData } = await utils.fetch<MemberMessage[]>({
            ...FETCH_TEMPLATE,
            url: FETCH_URL + `groups/${groupMsg.uuid}/members`,
            method: "GET"
        });

        if (jsonData) {
            const group: Group = {
                id: groupMsg.uuid,
                name: groupMsg.name,
                color: groupMsg.color,
                parent: "",
                memberHashes: jsonData.map(m => utils.genHash(m.uuid)),
            };

            groups.push(group);
        }
    }));

    return groups;
}

// === POST STUFF =========================================

async function fetchSetCurrentFronters(uid: string, toAddApiUids: string[]) {
    const { jsonData } = await utils.fetch<SwitchMessage>({
        ...FETCH_TEMPLATE,
        url: FETCH_URL + `systems/${uid}/switches`,
        method: "POST",
        body: {
            members: toAddApiUids,
        }
    });

    if (!jsonData) {
        return undefined;
    }

    const members = jsonData.members as MemberMessage[];

    return members.map(m => ({
        frontableApiUid: m.uuid,
        frontableHash: utils.genHash(m.uuid),
        startTime: new Date(jsonData.timestamp).getTime(),
    }));
}

// === CONFIG STUFF =======================================

const CONFIG_LAYOUT: ConfigLayout = Object.freeze({
    tokenField: {
        label: "PluralKit Token - REQUIRED FOR APP TO WORK!",
        description: "Can be found with the [[ <em>pk;token</em> ]] command!"
    },
});

// === IMPL ASSEMBLING ====================================

const IMPL: APIImpl = Object.freeze({
    name: "PluralKit",
    setToken: (token: string) => FETCH_TEMPLATE.auth.token = token,
    endpoints: {
        fetchGetUID,
        fetchGetAllFrontables,
        fetchGetCurrentFronters,
        fetchGetGroups,
        fetchSetCurrentFronters,
    },
    configLayout: CONFIG_LAYOUT,
});

export default IMPL;
