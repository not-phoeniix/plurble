import * as utils from "./utils";

export interface ApiMessage<T = any> {
    exists: boolean;
    id: string;
    content: T
}

export interface SocketMessage<T = any> {
    msg: "Successfully authenticated" | "update" | "Authentication violation: Token is missing or invalid. Goodbye :)";
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

export interface Group {
    id: string;
    name: string;
    color: string;
    parent: string;
    members: string[];
}

export namespace Group {
    export function create(jsonData: GroupMessage): Group {
        const group: Group = jsonData.content;
        group.id = jsonData.id;
        return group;
    }
}

// message types
export type FrontEntryMessage = ApiMessage<FrontEntry>;
export type MemberMessage = ApiMessage<Member>;
export type CustomFrontMessage = ApiMessage<CustomFront>;
export type FrontEntrySocketMessage = SocketMessage<FrontEntry>;
export type GroupMessage = ApiMessage<Group>;
export type AuthSocketMessage = SocketMessage<undefined> & {
    resolvedToken: {
        uid: string;
        accessType: number;
        jwt: false;
    };
};

export enum ErrorCode {
    APIKeyInvalid = 1,
}

// describes all the message keys defined in package.json
export type AppMessageDesc = {
    PluralApiKey?: string;
    ApiKeyValid?: boolean;
    ErrorMessage?: string;

    NumCurrentFronters?: number;
    NumCurrentFrontersInBatch?: number;
    CurrentFronter?: number[];

    NumTotalFrontables?: number;
    NumFrontablesInBatch?: number;
    FrontableHash?: number[];
    FrontableName?: string;
    FrontableColor?: number[];
    FrontablePronouns?: string;
    FrontableIsCustom?: number[];
    FrontableGroupBitField?: number[];

    NumTotalGroups?: number;
    NumGroupsInBatch?: number;
    GroupName?: string;
    GroupColor?: number[];
    GroupParentIndex?: number[];

    AddFrontRequest?: number;
    SetFrontRequest?: number;
    RemoveFrontRequest?: number;
    FetchDataRequest?: boolean;
    ClearCacheRequest?: boolean;
};
