import { AppMessageDesc, Frontable, FrontEntryMessage, Group, Member } from "./types";
import * as utils from "./utils";

//! NOTE: make sure these match up with the #defines in 
//!   frontable.h & group.h <3
const FRONTABLES_PER_MESSAGE = 32;
const CURRENT_FRONTS_PER_MESSAGE = 16;
const GROUPS_PER_MESSAGE = 1;
const FRONTABLE_NAME_LENGTH = 32;
const FRONTABLE_PRONOUNS_LENGTH = 16;
const GROUP_NAME_LENGTH = 32;
const GROUP_LIST_MAX_COUNT = 32;
const DELIMETER = ';';

function assembleFrontableMessages(frontables: Frontable[], groups: Group[]) {
    // sort based on frontable type and name alphabetically
    frontables.sort((a, b) => {
        let value = 0;

        // sort members first
        const aIsMember = "pronouns" in a;
        const bIsMember = "pronouns" in b;
        if (aIsMember && !bIsMember) {
            value -= 5;
        } else if (!aIsMember && bIsMember) {
            value += 5;
        }

        // sort alphabetically
        if (a.name.toLowerCase() > b.name.toLowerCase()) {
            value += 1;
        } else if (a.name.toLowerCase() < b.name.toLowerCase()) {
            value -= 1;
        }

        return value;
    });

    const numFrontables = frontables.length;
    const numMessages = Math.ceil(numFrontables / FRONTABLES_PER_MESSAGE);

    const messages: AppMessageDesc[] = [];

    for (let i = 0; i < numMessages; i++) {
        const batchSize = Math.min(frontables.length, FRONTABLES_PER_MESSAGE);
        const toSend = frontables.splice(0, batchSize);

        let namesArr: string[] = [];
        let pronounsArr: string[] = [];
        let colorsArr: number[] = [];
        let isCustomArr: boolean[] = [];
        let hashesArr: number[] = [];
        let groupBitArr: number[] = [];

        toSend.forEach((frontable) => {
            // store pronouns
            const member = frontable as Member;
            if (member.pronouns) {
                let pronouns = member.pronouns;
                if (pronouns.length > FRONTABLE_PRONOUNS_LENGTH) {
                    pronouns = pronouns.slice(0, FRONTABLE_PRONOUNS_LENGTH);
                }
                pronounsArr.push(pronouns);
            } else {
                pronounsArr.push("");
            }

            // store name
            let name = frontable.name;
            if (name.length > FRONTABLE_NAME_LENGTH) {
                name = name.slice(0, FRONTABLE_NAME_LENGTH);
            }
            namesArr.push(name);

            // store colors
            colorsArr.push(utils.toARGB8Color(frontable.color));

            // store is custom
            isCustomArr.push(!("pronouns" in member));

            // store hashes
            hashesArr.push(frontable.hash);

            // make and store bit fields
            let bitField = 0;
            if (groups) {
                for (let i = 0; i < Math.min(groups.length, GROUP_LIST_MAX_COUNT); i++) {
                    if (groups[i].members.find(m => m === frontable.id)) {
                        bitField |= (1 << i);
                    }
                }
            }
            groupBitArr.push(bitField);
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
    }

    if (messages.length === 0) {
        messages.push({ NumTotalFrontables: 0 });
    }

    return messages;
}

function assembleGroupMessages(groups: Group[]) {
    const numGroups = groups.length;
    const numMessages = Math.ceil(numGroups / GROUPS_PER_MESSAGE);

    const groupsOriginal: Group[] = JSON.parse(JSON.stringify(groups));

    const messages: AppMessageDesc[] = [];

    for (let i = 0; i < numMessages; i++) {
        const batchSize = Math.min(groups.length, GROUPS_PER_MESSAGE);
        const toSend = groups.splice(0, batchSize);

        let namesArr: string[] = [];
        let colorsArr: number[] = [];
        let parentIndicesArr: number[] = [];

        toSend.forEach((group) => {
            // store name
            let name = group.name;
            if (name.length > GROUP_NAME_LENGTH) {
                name = name.slice(0, GROUP_NAME_LENGTH);
            }
            namesArr.push(name);

            // store colors
            colorsArr.push(utils.toARGB8Color(group.color));

            // store parent indices
            let index = -1;
            for (let j = 0; j < groupsOriginal.length; j++) {
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
    }

    if (messages.length === 0) {
        messages.push({ NumTotalGroups: 0 });
    }

    return messages;
}

function assembleCurrentFrontMessages(currentFronters: FrontEntryMessage[]) {
    const numFronters = currentFronters.length;
    const numMessages = Math.ceil(numFronters / CURRENT_FRONTS_PER_MESSAGE);

    const messages: AppMessageDesc[] = [];

    for (let i = 0; i < numMessages; i++) {
        const batchSize = Math.min(currentFronters.length, CURRENT_FRONTS_PER_MESSAGE);
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
