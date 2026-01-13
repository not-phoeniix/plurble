import { AppMessageDesc, Frontable, FrontEntryMessage, Group, Member } from "./types";
import * as utils from "./utils";
import * as sorting from "./sorting";

//! NOTE: make sure these match up with the #defines in 
//!   frontable.h & group.h <3
const FRONTABLES_PER_MESSAGE = 32;
const CURRENT_FRONTS_PER_MESSAGE = 16;
const GROUPS_PER_MESSAGE = 32;
const FRONTABLE_NAME_LENGTH = 32;
const FRONTABLE_PRONOUNS_LENGTH = 16;
const GROUP_NAME_LENGTH = 32;
const FRONTABLES_MAX_COUNT = 200;
const GROUP_LIST_MAX_COUNT = 32;
// const FRONTABLES_MAX_COUNT = 128;
// const GROUP_LIST_MAX_COUNT = 64;
const DELIMETER = ';';
const DEFAULT_COLOR = "#000000";

function assembleFrontableMessages(frontables: Frontable[], groups: Group[]) {
    frontables = sorting.sortFrontables(frontables);

    const numFrontables = Math.min(frontables.length, FRONTABLES_MAX_COUNT);
    const numMessages = Math.ceil(numFrontables / FRONTABLES_PER_MESSAGE);

    const messages: AppMessageDesc[] = [];

    let frontablesRemaining = numFrontables;
    for (let i = 0; i < numMessages; i++) {
        const batchSize = Math.min(frontablesRemaining, FRONTABLES_PER_MESSAGE);
        const toSend = frontables.splice(0, batchSize);

        let namesArr: string[] = [];
        let pronounsArr: string[] = [];
        let colorsArr: number[] = [];
        let isCustomArr: boolean[] = [];
        let hashesArr: number[] = [];
        let groupBitArr: number[] = [];

        toSend.forEach((frontable) => {
            const member = frontable as Member;

            // store pronouns
            let pronouns = "";
            if (member.pronouns) {
                pronouns = utils.cleanString(member.pronouns, FRONTABLE_PRONOUNS_LENGTH);
            }
            pronounsArr.push(pronouns);

            // store name
            let name = utils.cleanString(frontable.name, FRONTABLE_NAME_LENGTH);
            namesArr.push(name);

            // store colors
            let color = DEFAULT_COLOR;
            if (frontable.color) {
                color = frontable.color;
            }
            colorsArr.push(utils.toARGB8Color(color));

            // store is custom
            isCustomArr.push(frontable.isCustom);

            // store hashes
            hashesArr.push(frontable.hash);

            // make and store bit fields dynamically
            //   (hardcoding 32 bit integers here)
            const numBitFields = Math.ceil(GROUP_LIST_MAX_COUNT / 32);
            const groupCount = Math.min(groups.length, GROUP_LIST_MAX_COUNT);
            for (let i = 0; i < numBitFields; i++) {
                let bitField = 0;

                if (groups) {
                    // iterate from the current 32 bits to the next 32 bits
                    for (let j = (i * 32); j < ((i + 1) * 32); j++) {
                        if (j >= groupCount) break;

                        if (groups[j].members.find(m => m === frontable.id)) {
                            bitField |= (1 << (j % 32));
                        }
                    }
                }

                groupBitArr.push(bitField);
            }
        });

        const msg: AppMessageDesc = {
            FrontableHash: utils.toByteArray(hashesArr),
            FrontableName: namesArr.map(n => n.replace(DELIMETER, "_")).join(DELIMETER),
            FrontablePronouns: pronounsArr.map(p => p.replace(DELIMETER, "_")).join(DELIMETER),
            FrontableIsCustom: isCustomArr.map(c => c ? 1 : 0),
            FrontableColor: colorsArr,
            FrontableGroupBitField: utils.toByteArray(groupBitArr),
            NumFrontablesInBatch: batchSize
        };

        if (i === 0) {
            // signal we are at the start of a batch by 
            //   specifying size only with the first message
            msg.NumTotalFrontables = numFrontables;
        }

        messages.push(msg);
        frontablesRemaining -= batchSize;
    }

    if (messages.length === 0) {
        messages.push({ NumTotalFrontables: 0 });
    }

    return messages;
}

function assembleGroupMessages(groups: Group[]) {
    groups = sorting.sortGroups(groups);

    const numGroups = Math.min(groups.length, GROUP_LIST_MAX_COUNT);
    const numMessages = Math.ceil(numGroups / GROUPS_PER_MESSAGE);

    const groupsOriginal: Group[] = JSON.parse(JSON.stringify(groups));

    const messages: AppMessageDesc[] = [];

    let groupsRemaining = numGroups;
    for (let i = 0; i < numMessages; i++) {
        const batchSize = Math.min(groupsRemaining, GROUPS_PER_MESSAGE);
        const toSend = groups.splice(0, batchSize);

        let namesArr: string[] = [];
        let colorsArr: number[] = [];
        let parentIndicesArr: number[] = [];

        toSend.forEach((group) => {
            // store name
            let name = utils.cleanString(group.name, GROUP_NAME_LENGTH);
            namesArr.push(name);

            // store colors
            let color = DEFAULT_COLOR;
            if (group.color) {
                color = group.color;
            }
            colorsArr.push(utils.toARGB8Color(color));

            // store parent indices
            let index = -1;
            for (let j = 0; j < numGroups; j++) {
                // don't use indices that won't fit in 8 bits
                if (j >= 255) break;

                if (group.parent === groupsOriginal[j].id) {
                    index = j;
                    break;
                }
            }

            // +1 the index so we can fit negative 1 within an unsigned int
            parentIndicesArr.push(index + 1);
        });

        const msg: AppMessageDesc = {
            GroupName: namesArr.map(n => n.replace(DELIMETER, "_")).join(DELIMETER),
            GroupColor: colorsArr,
            GroupParentIndex: parentIndicesArr,

            // no need for byte array, batch size should always 
            //   be below max value of uint8_t (below 255)
            NumGroupsInBatch: batchSize
        };

        if (i === 0) {
            // signal we are at the start of a batch by 
            //   specifying size only with the first message
            msg.NumTotalGroups = numGroups;
        }

        messages.push(msg);
        groupsRemaining -= batchSize;
    }

    if (messages.length === 0) {
        messages.push({ NumTotalGroups: 0 });
    }

    return messages;
}

function assembleCurrentFrontMessages(currentFronters: FrontEntryMessage[]) {
    const numFronters = Math.min(currentFronters.length, FRONTABLES_MAX_COUNT);
    const numMessages = Math.ceil(numFronters / CURRENT_FRONTS_PER_MESSAGE);

    const messages: AppMessageDesc[] = [];

    let frontersRemaining = numFronters;
    for (let i = 0; i < numMessages; i++) {
        const batchSize = Math.min(frontersRemaining, CURRENT_FRONTS_PER_MESSAGE);
        const toSend = currentFronters.splice(0, batchSize);

        const hashes = utils.toByteArray(
            toSend.map(entry => utils.genHash(entry.content.member))
        );

        const msg: AppMessageDesc = {
            CurrentFronter: hashes,
            NumCurrentFrontersInBatch: batchSize
        };

        if (i === 0) {
            // signal we are at the start of a batch by 
            //   specifying size only with the first message
            msg.NumCurrentFronters = numFronters;
        }

        messages.push(msg);
        frontersRemaining -= batchSize;
    }

    if (messages.length === 0) {
        messages.push({ NumCurrentFronters: 0 });
    }

    return messages;
}

export async function sendFrontablesToWatch(frontables: Frontable[], groups: Group[]): Promise<void> {
    const messages = assembleFrontableMessages(frontables, groups);

    for (let msg of messages) {
        await (PebbleTS.sendAppMessage(msg)
            .then(
                () => console.log("Frontable data successfully sent !!"),
                (reason) => console.log("Frontable data sending failed !! reason: " + reason)
            ));
    }
}

export async function sendCurrentFrontersToWatch(currentFronters: FrontEntryMessage[]): Promise<void> {
    const messages = assembleCurrentFrontMessages(currentFronters);

    for (let msg of messages) {
        await PebbleTS.sendAppMessage(msg)
            .then(
                () => console.log("Current front data successfully sent !!"),
                (reason) => console.log("Current front data sending failed !! reason: " + reason)
            );
    }
}

export async function sendGroupsToWatch(groups: Group[]): Promise<void> {
    const messages = assembleGroupMessages(groups);

    for (const msg of messages) {
        await PebbleTS.sendAppMessage(msg)
            .then(
                () => console.log("Group data successfully sent !!"),
                (reason) => console.log("Group data sending failed !! reason: " + reason)
            );
    }
}

export async function sendDataBatchToWatch(
    frontables: Frontable[],
    currentFronters: FrontEntryMessage[],
    groups: Group[],
): Promise<void> {
    const messages: AppMessageDesc[] = [];

    // assemble and merge frontable messages
    const frontableMessages = assembleFrontableMessages(frontables, groups);
    messages.push(...frontableMessages);

    // assemble and merge all current frontable message data
    const currentFronterMessages = assembleCurrentFrontMessages(currentFronters);
    for (let i = 0; i < currentFronterMessages.length; i++) {
        if (i >= messages.length) {
            messages.push(currentFronterMessages[i]);
        } else {
            // object spreading to merge the properties of both objects
            messages[i] = { ...messages[i], ...currentFronterMessages[i] };
        }
    }

    // assemble and merge all group message data
    const groupMessages = assembleGroupMessages(groups);
    for (let i = 0; i < groupMessages.length; i++) {
        if (i >= messages.length) {
            messages.push(groupMessages[i]);
        } else {
            messages[i] = { ...messages[i], ...groupMessages[i] };
        }
    }

    // take all the merged data and send it all :D
    for (let msg of messages) {
        await PebbleTS.sendAppMessage(msg);
    }
}

export async function sendApiKeyIsValid(valid: boolean): Promise<void> {
    return PebbleTS.sendAppMessage(<AppMessageDesc>{
        ApiKeyValid: valid,
    });
}

export async function sendErrorMessage(message: string): Promise<void> {
    return PebbleTS.sendAppMessage(<AppMessageDesc>{
        ErrorMessage: message
    });
}
