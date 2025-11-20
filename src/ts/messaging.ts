import { AppMessageDesc, Frontable, FrontEntryMessage, Group, Member } from "./types";
import * as utils from "./utils";

//! NOTE: make sure these match up with the #define's in 
//!   messaging.c, frontable.h, & group.h <3
const FRONTABLES_PER_MESSAGE = 32;
const CURRENT_FRONTS_PER_MESSAGE = 16;
const GROUPS_PER_MESSAGE = 16;
const FRONTABLE_NAME_LENGTH = 32;
const FRONTABLE_PRONOUNS_LENGTH = 16;
const GROUP_NAME_LENGTH = 32;
const DELIMETER = ';';

export async function sendFrontablesToWatch(frontables: Frontable[]): Promise<void> {
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
        } else if (a.name.toLocaleLowerCase() < b.name.toLowerCase()) {
            value -= 1;
        }

        return value;
    });

    const numFrontables = frontables.length;
    const numMessages = Math.ceil(numFrontables / FRONTABLES_PER_MESSAGE);

    for (let i = 0; i < numMessages; i++) {
        const batchSize = Math.min(frontables.length, FRONTABLES_PER_MESSAGE);
        const toSend = frontables.splice(0, batchSize);

        let namesArr: string[] = [];
        let pronounsArr: string[] = [];
        let colorsArr: number[] = [];
        let isCustomArr: boolean[] = [];
        let hashesArr: number[] = [];

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
        });

        const msg: AppMessageDesc = {
            FrontableHash: utils.toByteArray(hashesArr),
            FrontableName: namesArr.map(n => n.replace(DELIMETER, "_")).join(DELIMETER),
            FrontablePronouns: pronounsArr.map(p => p.replace(DELIMETER, "_")).join(DELIMETER),
            FrontableIsCustom: isCustomArr.map(c => c ? 1 : 0),
            FrontableColor: colorsArr,

            // no need for byte array, batch size should always 
            //   be below max value of uint8_t (below 255)
            NumFrontablesInBatch: batchSize
        };

        if (i === 0) {
            // signal we are at the start of a batch by 
            //   specifying size only with the first message
            msg.NumTotalFrontables = numFrontables;
        }

        console.log(`Sending ${batchSize} frontables in a batch to watch...`);

        await PebbleTS.sendAppMessage(msg)
            .then(
                () => console.log("Frontable data successfully sent !!"),
                (reason) => console.log("Frontable data sending failed !! reason: " + reason)
            );
    }
}

export async function sendCurrentFrontersToWatch(currentFronters: FrontEntryMessage[]): Promise<void> {
    const numFronters = currentFronters.length;
    const numMessages = Math.ceil(numFronters / CURRENT_FRONTS_PER_MESSAGE);

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

        await PebbleTS.sendAppMessage(msg);
    }

    if (numFronters === 0) {
        await PebbleTS.sendAppMessage(<AppMessageDesc>{
            NumCurrentFronters: 0
        });
    }
}

export async function sendGroupsToWatch(groups: Group[]): Promise<void> {
    const numGroups = groups.length;
    const numMessages = Math.ceil(numGroups / GROUPS_PER_MESSAGE);

    for (let i = 0; i < numMessages; i++) {
        const batchSize = Math.min(groups.length, GROUPS_PER_MESSAGE);
        const toSend = groups.splice(0, batchSize);

        let namesArr: string[] = [];
        let colorsArr: number[] = [];
        let membersArr: number[] = [];
        let parentIndicesArr: number[] = [];

        toSend.forEach((group, i) => {
            // store name
            let name = group.name;
            if (name.length > GROUP_NAME_LENGTH) {
                name = name.slice(0, GROUP_NAME_LENGTH);
            }
            namesArr.push(name);

            // store colors
            colorsArr.push(utils.toARGB8Color(group.color));

            // store members (weird format)
            //
            // members array format
            //   [
            //      memberCount, 
            //      hash0_byte0, hash0_byte1, hash0_byte2, hash0_byte3 
            //      hash1_byte0, hash1_byte1, hash1_byte2, hash1_byte3 
            //      ... 
            //                                   hashMemberCount_byte3
            //   ]
            // 
            // ensure member array doesn't exceed 256 byte limit for message keys
            const numMembers = Math.min(group.members.length, Math.floor(255 / 4));
            if (membersArr.length + (numMembers * 4) + 1 <= 256) {
                // only add to array if it stays below the 256 count limit
                membersArr.push(numMembers);
                for (let i = 0; i < numMembers; i++) {
                    const memberHash = utils.genHash(group.members[i]);
                    membersArr.push(...utils.toByteArray([memberHash]));
                }
            }

            // store parent indices
            let index = -1;
            for (let j = 0; j < toSend.length; j++) {
                if (i === j) continue;

                // don't use indices that won't fit in 8 bits
                if (j >= 255) break;

                if (group.parent === toSend[j].id) {
                    index = j;
                    break;
                }
            }

            // +1 the index so we can fit it within an unsigned int
            parentIndicesArr.push(index + 1);
        });

        const msg: AppMessageDesc = {
            GroupName: namesArr.map(n => n.replace(DELIMETER, "_")).join(DELIMETER),
            GroupColor: colorsArr,
            GroupMembers: membersArr,
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

        console.log(`Sending ${batchSize} groups in a batch to watch...`);

        await PebbleTS.sendAppMessage(msg)
            .then(
                () => console.log("Group data successfully sent !!"),
                (reason) => console.log("Group data sending failed !! reason: " + reason)
            );
    }
}

export async function sendApiKeyIsValid(valid: boolean): Promise<void> {
    return PebbleTS.sendAppMessage(<AppMessageDesc>{
        ApiKeyValid: valid,
    });
}
