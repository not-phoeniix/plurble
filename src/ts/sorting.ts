import { Frontable, FrontEntryMessage, Group } from "./types";
import * as utils from "./utils";

export function sortFrontables(frontables: Frontable[]): Frontable[] {
    let output: Frontable[] = JSON.parse(JSON.stringify(frontables));

    // sort based on frontable type and name alphabetically
    output.sort((a, b) => {
        let value = 0;

        // sort custom fronts first
        if (a.isCustom && !b.isCustom) {
            value -= 5;
        } else if (!a.isCustom && b.isCustom) {
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

    return output;
}

interface GroupNode {
    parent: GroupNode | null;
    group: Group;
    children: GroupNode[];
}

interface NodeDictionary {
    [key: string]: GroupNode
}

export function sortGroups(groups: Group[], frontables: Frontable[]): Group[] {
    // create and assemble dictionary that maps group id's to new nodes
    const nodeMap: NodeDictionary = {};
    groups.forEach(g => {
        nodeMap[g.id] = {
            parent: null,
            group: g,
            children: []
        };
    });

    // re-iterate to set up the tree itself
    const rootChildren: GroupNode[] = [];
    for (const group of groups) {
        const node = nodeMap[group.id];
        const parent = nodeMap[group.parent];

        if (parent) {
            node.parent = parent;
            parent.children.push(node);
        } else {
            rootChildren.push(node);
        }
    }

    // sort alphabetically
    function sort(a: GroupNode, b: GroupNode) {
        if (a.group.name.toLowerCase() > b.group.name.toLowerCase()) {
            return 1;
        } else if (a.group.name.toLowerCase() < b.group.name.toLowerCase()) {
            return -1;
        }

        return 0;
    }


    const output: Group[] = [];

    function addNode(node: GroupNode) {
        // add self, sort children, then add children
        output.push(node.group);
        node.children.sort(sort);
        for (const child of node.children) {
            addNode(child);
        }
    }

    rootChildren.sort(sort);
    for (const node of rootChildren) {
        addNode(node);
    }

    // sort group children alphabetically
    groups.forEach(g => g.members.sort((a, b) => {
        const aHash = utils.genHash(a);
        const bHash = utils.genHash(b);
        const memberA = frontables.find(f => f.hash === aHash);
        const memberB = frontables.find(f => f.hash === bHash);

        if (!memberA || !memberB) return 0;

        if (memberA.name.toLowerCase() > memberB.name.toLowerCase()) {
            return 1;
        } else if (memberA.name.toLowerCase() < memberB.name.toLowerCase()) {
            return -1;
        }

        return 0;
    }));

    return output;
}

export function sortCurrentFronts(currentFronts: FrontEntryMessage[]): FrontEntryMessage[] {
    let output: FrontEntryMessage[] = JSON.parse(JSON.stringify(currentFronts));

    output.sort((a, b) => {
        const aStart = a.content.startTime ?? 0;
        const bStart = b.content.startTime ?? 0;
        return bStart - aStart;
    });

    return output;
}
