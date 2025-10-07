import * as utils from "./utils";

export interface ApiMessage<T = any> {
    exists: boolean;
    id: string;
    content: T
}

export interface SocketMessage<T = any> {
    msg: "Successfully authenticated" | "update";
    target?: "frontHistory";
    results: {
        exists: boolean;
        id: string;
        operationType: "update" | "insert";
        content: T;
    }[];
}

export interface Member {
    name: string;
    uid: string;
    id: string;
    avatarUrl: string;
    desc: string;
    color: string;
    pronouns: string;
    archived: boolean;
    hash: number;
};

export namespace Member {
    export function create(jsonData: MemberMessage): Member {
        const member: Member = jsonData.content;
        member.id = jsonData.id;
        member.hash = utils.genHash(member.id);
        return member;
    }
}

export interface CustomFront {
    name: string;
    uid: string;
    id: string;
    avatarUrl: string;
    desc: string;
    color: string;
    hash: number;
};

export namespace CustomFront {
    export function create(jsonData: CustomFrontMessage): CustomFront {
        const customFront: CustomFront = jsonData.content;
        customFront.id = jsonData.id;
        customFront.hash = utils.genHash(customFront.id);
        return customFront;
    }
}

export interface FrontEntry {
    live: boolean;
    member: string;
    custom: boolean;
    customStatus: string;
    startTime?: number;
    endTime?: number;
}

export type Frontable = Member | CustomFront;

// message types
export type FrontEntryMessage = ApiMessage<FrontEntry>;
export type MemberMessage = ApiMessage<Member>;
export type CustomFrontMessage = ApiMessage<CustomFront>;
export type FrontEntrySocketMessage = SocketMessage<FrontEntry>;
export type AuthSocketMessage = SocketMessage<undefined> & {
    resolvedToken: {
        uid: string;
        accessType: number;
        jwt: false;
    };
};
