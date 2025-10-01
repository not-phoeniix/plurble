import { Frontable, FrontableCollection, Member } from "./types";

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
            "FrontableIsCustom": false
        };

        const member = frontable as Member;
        if (member.pronouns) {
            msg["FrontablePronouns"] = member.pronouns;
            msg["FrontableIsCustom"] = true;
        }

        if (i == 0) {
            msg["NumTotalFrontables"] = Object.keys(frontables).length;
        }

        await PebbleTS.sendAppMessage(msg);
        i++;
    }
}

export async function sendCurrentFrontersToWatch(currentFronterHashes: number[]): Promise<void> {
    const msg: Record<string, any> = {
        "CurrentFronters": currentFronterHashes
    };

    return PebbleTS.sendAppMessage(msg);
}
