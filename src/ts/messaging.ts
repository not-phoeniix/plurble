import { Frontable, FrontEntryMessage, Member } from "./types";
import * as utils from "./utils";

export async function sendFrontablesToWatch(frontables: Frontable[]): Promise<void> {
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
        if (a.name > b.name) {
            value += 1;
        } else if (a.name < b.name) {
            value -= 1;
        }

        return value;
    });

    let i = 0;
    for (const frontable of frontables) {
        // for (const [hash, frontable] of Object.entries(frontables)) {
        //* apparently strtol doesn't exist on pebble 
        //*   lol so i need to use atoi using base 
        //*   10 numbers <3

        const msg: Record<string, any> = {
            // offset hash by half the max to make it 
            //   within signed bounds for messaging
            "FrontableHash": frontable.hash - Math.floor(0xFFFFFFFF / 2),
            "FrontableName": frontable.name,
            "FrontableColor": Number(`0x${frontable.color.replace("#", "")}`),
        };

        const member = frontable as Member;
        if (member.pronouns) {
            msg["FrontablePronouns"] = member.pronouns;
            msg["FrontableIsCustom"] = false;
        } else {
            msg["FrontableIsCustom"] = true;
        }

        if (i === 0) {
            msg["NumTotalFrontables"] = frontables.length;
        }

        await PebbleTS.sendAppMessage(msg)
            .then(
                () => console.log("Frontable data successfully sent !!"),
                (reason) => console.log("Frontable data sending failed !! reason: " + reason)
            );
        i++;
    }
}

export async function sendCurrentFrontersToWatch(currentFronters: FrontEntryMessage[]): Promise<void> {
    for (let i = 0; i < currentFronters.length; i++) {
        const fronter = currentFronters[i];

        const msg: Record<string, any> = {
            "CurrentFronter": utils.genHash(fronter.content.member)
        };

        if (i === 0) {
            msg["NumCurrentFronters"] = currentFronters.length;
        }

        await PebbleTS.sendAppMessage(msg);
    }
}

export async function sendApiKeyIsValid(valid: boolean): Promise<void> {
    return PebbleTS.sendAppMessage({
        "ApiKeyValid": valid
    });
}
