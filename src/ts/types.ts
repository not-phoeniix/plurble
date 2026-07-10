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
    avatarUrl?: string;
    color?: string;
    pronouns?: string;
    archived: boolean;
    hash: number;
    isCustom: false;
}

export interface CustomFront {
    name: string;
    avatarUrl?: string;
    color?: string;
    hash: number;
    isCustom: true;
};

export interface FrontEntry {
    live: boolean;
    member: string;
    custom: boolean;
    customStatus: string;
    startTime?: number;
    endTime?: number;
}

export interface FrontEntry2 {
    frontableHash: number;
    startTime?: number;
    endTime?: number;
}

export type Frontable = Member | CustomFront;

export interface Group {
    id: string;
    name: string;
    color?: string;
    parent: string;
    memberHashes: number[];
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
export interface AppMessageDesc {
    PluralApiKey?: string;
    ApiKeyValid?: boolean;
    ErrorMessage?: string;

    NumCurrentFronters?: number;
    NumCurrentFrontersInBatch?: number;
    CurrentFronter?: number[];
    CurrentFrontStartTime?: number[];

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
}

export interface EndpointImpl {
    fetchGetUID: () => Promise<string>;
    fetchGetAllFrontables: (uid: string) => Promise<Frontable[]>;
    fetchGetCurrentFronters: (uid: string) => Promise<FrontEntry2[]>;
    fetchGetGroups?: (uid: string) => Promise<Group[]>;
    fetchSetCurrentFronters?: (toSet: Frontable[]) => Promise<void>;
}

export interface SocketImpl {
    onFrontUpdate: (newFronts: Frontable[]) => void;
    onApiValid: (isValid: boolean) => void;
}

export interface APIImpl {
    name: string;
    setToken: (token: string) => void;
    endpoints: EndpointImpl;
    socket?: SocketImpl;
}
