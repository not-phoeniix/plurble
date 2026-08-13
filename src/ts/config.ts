import { APIImpl } from "./types";
import * as backends from "./backends";
import * as cache from "./cache";
import initialConfig from "./config.json";

let currentBackendStr = cache.getBackend() || "pluralKit";

function getCurrentBackend(): APIImpl {
    if (currentBackendStr in backends) {
        // weird type workarounds <//3
        return (backends as any)[currentBackendStr];
    }

    // pluralkit by default if the backend wasn't found
    return backends["pluralKit"];
}

// TODO: TS types for this would help a lot
function customClay(minified: any) {
    // you can ignore the "this" error, it'll go away when transpiling <3
    const clayConfig: any = this;

    function showCurrentBackendOptions(self: any) {
        console.log("showCurrentbackendOptions called...");

        let currentBackendStr = "unknown";

        if (self) {
            currentBackendStr = self.get();
        } else {
            console.warn("WARNING: showCurrentBackendOptions' \"self\" is falsy! backend str is unset!");
        }

        console.log(`current backend str: "${currentBackendStr}"`);

        // find all grouped items, show all matching and hide non matching
        clayConfig.getAllItems().forEach((item: any) => {
            const { group } = item.config;
            if (group) {
                if (group === currentBackendStr) {
                    item.show();
                } else {
                    item.hide();
                }
            }
        });
    }

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function () {
        const backendDropdown = clayConfig.getItemByMessageKey("Backend");
        showCurrentBackendOptions(backendDropdown);
        backendDropdown.on("change", () => showCurrentBackendOptions(backendDropdown));
    });
}

function onShowConfig(clay: any) {
    let allItems = [];
    const backendDropdownOptions: { label: string; value: string; }[] = [];

    for (let backendStr in backends) {
        console.log(`found backend: "${backendStr}"`);

        // i know, "as any" is awful practice, ill probs refactor later
        const backend: APIImpl = (backends as any)[backendStr];
        const { configLayout } = backend;

        // important to be able to find config options 
        //   associated with a particular backend
        const group = backendStr;

        // add an option for the dropdown
        backendDropdownOptions.push({
            label: backend.name,
            value: group,
        });

        // toss in the token input template
        allItems.push({
            type: "input",
            messageKey: "PluralApiKey",
            attributes: {
                type: "password"
            },
            group,
            ...configLayout.tokenField,
        });

        // also toss in any additional elements
        if (configLayout.additionalItems) {
            const groupedItems = configLayout.additionalItems
                .map(item => ({ ...item, group }));
            allItems.push(...groupedItems);
        }
    }

    allItems = [
        // put backend select first
        {
            type: "select",
            messageKey: "Backend",
            label: "Backend",
            description: "Which backend to use (more will be added soon!)",
            defaultValue: currentBackendStr,
            options: backendDropdownOptions,
        },
        ...allItems,
    ];

    // copy initial config, add items, assign to clay
    const newConfig = JSON.parse(JSON.stringify(initialConfig));
    newConfig[1].items.push(...allItems);
    clay.config = newConfig;

    // finally, load clay page
    Pebble.openURL(clay.generateUrl());
}

function init() {
    // i gotta use node CommonJS requires unfortunately, it's not a TS module
    const Clay = require("pebble-clay");
    const clay = new Clay(initialConfig, customClay, { autoHandleEvents: false });

    Pebble.addEventListener("showConfiguration", () => onShowConfig(clay));

    return clay;
}

export {
    init,
    getCurrentBackend,
};
