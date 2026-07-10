// TODO: remove this implementation/file

import { APIImpl, ApiMessage, CustomFront, Frontable, FrontEntryMessage, GroupMessage, Member, MemberMessage } from "../types";
import * as utils from "../utils";

const FETCH_URL = "https://api.apparyllis.com/v1/";
const FETCH_TEMPLATE = Object.seal({
    auth: {
        headerName: "Authorization",
        token: ""
    }
});

// === SIMPLE GET STUFF ===================================

async function fetchGetUID() {
    const { jsonData } = await utils.fetch<{ id: string }>({
        ...FETCH_TEMPLATE,
        url: FETCH_URL + "me/",
        method: "GET"
    })

    return jsonData?.id ?? "";
}

async function fetchGetAllFrontables(uid: string) {
    const members: Member[] = [];
    const customFronts: CustomFront[] = [];

    async function fetchFrontables<T extends Frontable>(
        url: string,
        isCustom: boolean,
        dstArray: T[]
    ) {
        const { jsonData } = await utils.fetch<ApiMessage<T>[]>({
            ...FETCH_TEMPLATE,
            url,
            method: "GET"
        });

        const frontables = jsonData?.map(msg => ({
            ...msg.content,
            isCustom,
            id: msg.id,
            hash: utils.genHash(msg.id),
        })) ?? [];

        dstArray.push(...frontables);
    }

    await Promise.all([
        // members
        fetchFrontables<Member>(
            FETCH_URL + `members/${uid}/`,
            false,
            members
        ),

        // custom fronts
        fetchFrontables<CustomFront>(
            FETCH_URL + `customFronts/${uid}/`,
            false,
            customFronts
        ),
    ]);

    return [...customFronts, ...members];
}

async function fetchGetCurrentFronters(uid: string) {
    const { jsonData } = await utils.fetch<FrontEntryMessage[]>({
        ...FETCH_TEMPLATE,
        url: FETCH_URL + `fronters/`,
        method: "GET"
    });

    return jsonData?.map(m => ({
        frontableHash: utils.genHash(m.content.member),
        startTime: m.content.startTime,
        endTime: m.content.endTime,
    })) ?? [];
}

async function fetchGetGroups(uid: string) {
    const { jsonData } = await utils.fetch<GroupMessage[]>({
        ...FETCH_TEMPLATE,
        url: FETCH_URL + `groups/${uid}`,
        method: "GET"
    });

    if (!jsonData) {
        return [];
    }

    return jsonData.map(m => ({ ...m.content, id: m.id }));
}

// === POST STUFF =========================================

// honestly porting this is kinda boring and it's not even 
//   up as a service anymore so it's not worth it

async function fetchSetCurrentFronters(toAdd: Frontable[]) {
}

// === IMPL ASSEMBLING ====================================

const IMPL: APIImpl = {
    name: "SimplyPlural",
    setToken: (token: string) => FETCH_TEMPLATE.auth.token = token,
    endpoints: {
        fetchGetUID,
        fetchGetAllFrontables,
        fetchGetCurrentFronters,
        fetchGetGroups,
        fetchSetCurrentFronters,
    },
};

export default IMPL;
