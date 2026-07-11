export interface Member {
    name: string;
    avatarUrl?: string;
    color?: string;
    pronouns?: string;
    archived: boolean;
    apiUid: string;
    hash: number;
    isCustom: false;
}

export interface CustomFront {
    name: string;
    avatarUrl?: string;
    color?: string;
    apiUid: string;
    hash: number;
    isCustom: true;
};

export interface FrontEntry {
    frontableApiUid: string;
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
    fetchGetCurrentFronters: (uid: string) => Promise<FrontEntry[]>;
    fetchGetGroups?: (uid: string) => Promise<Group[]>;
    fetchSetCurrentFronters: (uid: string, toSetApiUids: string[]) => Promise<FrontEntry[] | undefined>;
}

export interface SocketImpl {
    onFrontUpdate: (newFronts: Frontable[]) => void;
    onApiValid: (isValid: boolean) => void;
}

// TODO: figure out how to implement a cached switch queueing system 
//   (to avoid rate limiting for fast switches and other data fetching)
export interface APIImpl {
    name: string;
    setToken: (token: string) => void;
    endpoints: EndpointImpl;
    socket?: SocketImpl;
}
