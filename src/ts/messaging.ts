import { FrontableCollection, FrontEntryMessage, Member } from "./types";
import * as utils from "./utils";

export async function sendFrontablesToWatch(frontables: FrontableCollection): Promise<void> {
    let i = 0;

    for (const hash of Object.keys(frontables).map(k => Number(k))) {
        const frontable = frontables[hash];

        //* apparently strtol doesn't exist on pebble 
        //*   lol so i need to use atoi using base 
        //*   10 numbers <3

        const msg: Record<string, any> = {
            "FrontableHash": hash,
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
            msg["NumTotalFrontables"] = Object.keys(frontables).length;
            console.log("sending a total frontables number message.... this should reset cache ://");
        }

        console.log(`sending frontable "${frontable.name}"...`);
        console.log(`msg: ${JSON.stringify(msg)}`);
        await PebbleTS.sendAppMessage(msg)
            .then(
                () => console.log("front message successfully sent !!"),
                (reason) => console.log("front message failed or smth !! reason: " + reason)
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
