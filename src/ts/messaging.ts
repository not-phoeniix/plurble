import { AppMessageDesc, Frontable, FrontEntryMessage, Member } from "./types";
import * as utils from "./utils";

//! NOTE: make sure these match up with the #define's in 
//!   messaging.c and frontable.h <3
const FRONTABLES_PER_MESSAGE = 32;
const CURRENT_FRONTS_PER_MESSAGE = 16;
const NAME_LENGTH = 32;
const PRONOUNS_LENGTH = 16;
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

        toSend.forEach((frontable, i) => {
            // store pronouns
            const member = frontable as Member;
            if (member.pronouns) {
                let pronouns = member.pronouns;
                if (pronouns.length > PRONOUNS_LENGTH) {
                    pronouns = pronouns.slice(0, PRONOUNS_LENGTH);
                }
                pronounsArr.push(pronouns);
            } else {
                pronounsArr.push("");
            }

            // store name
            let name = frontable.name;
            if (name.length > NAME_LENGTH) {
                name = name.slice(0, NAME_LENGTH);
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

export async function sendApiKeyIsValid(valid: boolean): Promise<void> {
    return PebbleTS.sendAppMessage(<AppMessageDesc>{
        ApiKeyValid: valid,
    });
}
