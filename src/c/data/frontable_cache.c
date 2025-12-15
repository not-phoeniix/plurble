#include "frontable_cache.h"
#include "../tools/string_tools.h"

#define PRONOUNS_KEY 2
#define FRONTABLES_NUM_KEY 3
#define FRONTABLES_KEY_MIN 4
#define FRONTABLES_KEY_MAX 20
#define GROUPS_NUM_KEY 21
#define GROUPS_KEY_MIN 22
#define GROUPS_KEY_MAX 30

// tweak these to adjust how much memory is allocated
#define MAX_CACHED_FRONTABLES 96
#define MAX_CACHED_PRONOUNS 16
#define MAX_CACHED_GROUPS GROUP_LIST_MAX_COUNT
#define COMPRESSED_NAME_LENGTH 20
#define COMPRESSED_PRONOUNS_LENGTH 11

#define FRONTABLE_QUEUE_SIZE 512
#define GROUP_QUEUE_SIZE GROUP_LIST_MAX_COUNT
#define CURRENT_FRONTER_QUEUE_SIZE 512

// ~~~ current memory footprint ~~~
//
// max memory taken up by pronoun map:
//   MAX_CACHED_PRONOUNS * (COMPRESSED_PRONOUNS_LENGTH) == 176 bytes
//
// max memory taken up by frontables:
//   sizeof(CompressedFrontable) * MAX_CACHED_FRONTABLES == 3072 bytes
//   ^ this should equate to 12 chunks stored
//
// max memory taken up by groups:
//   sizeof(CompressedGroup) * MAX_CACHED_GROUPS == 704 bytes
//
// total memory footprint: 3952 bytes <3

typedef struct {
    char name[COMPRESSED_NAME_LENGTH];
    uint32_t hash;
    uint32_t group_bit_field;
    uint8_t pronoun_index;
    uint8_t packed_data;
} CompressedFrontable;

typedef struct {
    char name[COMPRESSED_NAME_LENGTH];
    uint8_t color;
    uint8_t parent_index;
} CompressedGroup;

// TODO: command cache for frontable tracking for when phone gets disconnected
// typedef struct {
//     uint32_t hash;
//     time_t time;
//     uint8_t type;
// } Command;

static FrontableList members;
static FrontableList custom_fronts;
static FrontableList current_fronters;
static GroupCollection groups;

static Frontable** frontable_queue = NULL;
static uint16_t frontable_queue_count = 0;
static Group** group_queue = NULL;
static uint16_t group_queue_count = 0;
static uint32_t* current_fronter_queue = NULL;
static uint16_t current_fronter_queue_count = 0;

FrontableList* cache_get_members() { return &members; }
FrontableList* cache_get_custom_fronts() { return &custom_fronts; }
FrontableList* cache_get_current_fronters() { return &current_fronters; }
Frontable* cache_get_first_fronter() {
    if (current_fronters.frontables != NULL) {
        return current_fronters.frontables[0];
    }

    return NULL;
}

Frontable* cache_get_frontable(uint32_t hash) {
    //! using just iteration to search isn't the best idea...
    //!   for the future, since we have hashes for each frontable:
    //!   make a hash map maybe !!!!

    for (uint32_t i = 0; i < members.num_stored; i++) {
        Frontable* member = members.frontables[i];
        if (member->hash == hash) {
            return member;
        }
    }

    for (uint32_t i = 0; i < custom_fronts.num_stored; i++) {
        Frontable* custom_front = custom_fronts.frontables[i];
        if (custom_front->hash == hash) {
            return custom_front;
        }
    }

    return NULL;
}

void cache_add_frontable(Frontable* frontable) {
    if (frontable_get_is_custom(frontable)) {
        frontable_list_add(frontable, &custom_fronts);
    } else {
        frontable_list_add(frontable, &members);
    }
}

void cache_clear_frontables() {
    frontable_list_deep_clear(&members);
    frontable_list_deep_clear(&custom_fronts);
}

void cache_add_current_fronter(uint32_t frontable_hash) {
    Frontable* frontable = cache_get_frontable(frontable_hash);
    if (frontable != NULL) {
        frontable_set_is_fronting(frontable, true);
        frontable_list_add(frontable, &current_fronters);
    }
}

void cache_clear_current_fronters() {
    for (uint16_t i = 0; i < current_fronters.num_stored; i++) {
        frontable_set_is_fronting(current_fronters.frontables[i], false);
    }

    frontable_list_clear(&current_fronters);
}

void cache_add_group(Group* group) {
    if (groups.num_stored >= GROUP_LIST_MAX_COUNT) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "ERROR! Cannot add group '%s' to cache, limit has been reached!", group->name);
        return;
    }

    groups.groups[groups.num_stored] = group;
    groups.num_stored++;
}

GroupCollection* cache_get_groups() {
    return &groups;
}

void cache_clear_groups() {
    for (uint16_t i = 0; i < groups.num_stored; i++) {
        group_destroy(groups.groups[i]);
        groups.groups[i] = NULL;
    }
    groups.num_stored = 0;
}

void cache_queue_add_frontable(Frontable* frontable) {
    if (frontable_queue_count >= FRONTABLE_QUEUE_SIZE) {
        APP_LOG(APP_LOG_LEVEL_WARNING, "WARNING: Cannot add to frontable queue, max count has been reached!");
        return;
    }

    if (frontable_queue == NULL) {
        frontable_queue = malloc(sizeof(Frontable*) * FRONTABLE_QUEUE_SIZE);
    }

    frontable_queue[frontable_queue_count] = frontable;
    frontable_queue_count++;
}

void cache_queue_add_group(Group* group) {
    if (group_queue_count >= GROUP_QUEUE_SIZE) {
        APP_LOG(APP_LOG_LEVEL_WARNING, "WARNING: Cannot add to group queue, max count has been reached!");
        return;
    }

    if (group_queue == NULL) {
        group_queue = malloc(sizeof(Group*) * GROUP_QUEUE_SIZE);
    }

    group_queue[group_queue_count] = group;
    group_queue_count++;
}

void cache_queue_add_current_fronter(uint32_t hash) {
    if (current_fronter_queue_count >= CURRENT_FRONTER_QUEUE_SIZE) {
        APP_LOG(APP_LOG_LEVEL_WARNING, "WARNING: Cannot add to current fronter queue, max count has been reached!");
        return;
    }

    if (current_fronter_queue == NULL) {
        current_fronter_queue = malloc(sizeof(uint32_t) * CURRENT_FRONTER_QUEUE_SIZE);
    }

    current_fronter_queue[current_fronter_queue_count] = hash;
    current_fronter_queue_count++;
}

void cache_queue_flush_frontables() {
    cache_clear_frontables();

    for (uint16_t i = 0; i < frontable_queue_count; i++) {
        // if current frontable is a member, add it to its groups
        Frontable* member = frontable_queue[i];
        if (!frontable_get_is_custom(member)) {
            for (uint16_t j = 0; j < groups.num_stored; j++) {
                // check bit field, if flag at that given index is 1
                //   add to the group at that index
                if (((member->group_bit_field >> j) & 1) != 0) {
                    frontable_list_add(member, groups.groups[j]->frontables);
                }
            }
        }

        printf("adding flush frontable %s", frontable_queue[i]->name);
        cache_add_frontable(frontable_queue[i]);
    }

    // iterate across members and add them to groups
    // GroupCollection* groups = cache_get_groups();
    // FrontableList* members = cache_get_members();
    // for (uint16_t i = 0; i < members->num_stored; i++) {
    //     Frontable* member = members->frontables[i];

    //     for (uint16_t j = 0; j < groups->num_stored; j++) {
    //         Group* group = groups->groups[j];

    //         if (((member->group_bit_field >> j) & 1) != 0) {
    //             frontable_list_add(member, group->frontables);
    //         }
    //     }
    // }

    frontable_queue_count = 0;
}

void cache_queue_flush_groups() {
    cache_clear_groups();

    for (uint16_t i = 0; i < group_queue_count; i++) {
        printf("adding flush group %s", group_queue[i]->name);
        cache_add_group(group_queue[i]);
    }

    group_queue_count = 0;
}

void cache_queue_flush_current_fronters() {
    cache_clear_current_fronters();

    for (uint16_t i = 0; i < current_fronter_queue_count; i++) {
        printf("adding flush current front %lu", current_fronter_queue[i]);
        cache_add_current_fronter(current_fronter_queue[i]);
    }

    current_fronter_queue_count = 0;
}

static void store_pronoun_map(char* pronoun_map, size_t pronoun_map_size) {
    uint16_t pronoun_index = 0;

    for (uint16_t i = 0; i < members.num_stored; i++) {
        Frontable* member = members.frontables[i];

        bool pronouns_exist = false;
        for (uint16_t j = 0; j < MAX_CACHED_PRONOUNS; j++) {
            char* pronouns = pronoun_map + (j * (COMPRESSED_PRONOUNS_LENGTH));
            if (string_start_same(member->pronouns, pronouns)) {
                pronouns_exist = true;
                break;
            }
        }

        // add to array if they haven't been cached yet
        if (!pronouns_exist && pronoun_index < MAX_CACHED_PRONOUNS) {
            string_safe_copy(
                pronoun_map + (pronoun_index * COMPRESSED_PRONOUNS_LENGTH),
                member->pronouns,
                COMPRESSED_PRONOUNS_LENGTH
            );
            pronoun_index++;
        }
    }

    persist_write_data(PRONOUNS_KEY, pronoun_map, pronoun_map_size);
}

static void store_frontables(char* pronoun_map) {
    CompressedFrontable* frontables_to_store = (CompressedFrontable*)malloc(
        sizeof(CompressedFrontable) * MAX_CACHED_FRONTABLES
    );
    uint16_t frontable_index = 0;

    // convert custom fronts into compressed frontables
    for (uint16_t i = 0; i < custom_fronts.num_stored; i++) {
        if (frontable_index >= MAX_CACHED_FRONTABLES) break;

        Frontable* custom_front = custom_fronts.frontables[i];

        frontables_to_store[frontable_index] = (CompressedFrontable) {
            .hash = custom_front->hash,
            .packed_data = custom_front->packed_data,
            .pronoun_index = 0,
            .group_bit_field = 0
        };
        string_safe_copy(
            frontables_to_store[frontable_index].name,
            custom_front->name,
            COMPRESSED_NAME_LENGTH
        );

        frontable_index++;
    }

    // convert members into compressed frontables
    for (uint16_t i = 0; i < members.num_stored; i++) {
        if (frontable_index >= MAX_CACHED_FRONTABLES) break;

        Frontable* member = members.frontables[i];

        // find index of pronouns, pronoun_index counts the
        //   number of stored pronouns at this point
        uint8_t pronoun_index = 0;
        for (uint16_t j = 0; j < MAX_CACHED_PRONOUNS; j++) {
            if (string_start_same(member->pronouns, pronoun_map + (j * (COMPRESSED_PRONOUNS_LENGTH)))) {
                pronoun_index = j + 1;
                break;
            }
        }

        frontables_to_store[frontable_index] = (CompressedFrontable) {
            .hash = member->hash,
            .packed_data = member->packed_data,
            .pronoun_index = pronoun_index,
            .group_bit_field = member->group_bit_field
        };
        string_safe_copy(
            frontables_to_store[frontable_index].name,
            member->name,
            COMPRESSED_NAME_LENGTH
        );

        frontable_index++;
    }

    // store chunks
    int32_t remaining_frontables = frontable_index;
    persist_write_int(FRONTABLES_NUM_KEY, remaining_frontables);
    uint16_t index_offset = 0;
    for (uint32_t key = FRONTABLES_KEY_MIN; key <= FRONTABLES_KEY_MAX; key++) {
        if (remaining_frontables <= 0) break;

        uint16_t num_to_store = PERSIST_DATA_MAX_LENGTH / sizeof(CompressedFrontable);
        if (num_to_store > remaining_frontables) num_to_store = remaining_frontables;
        uint16_t size = num_to_store * sizeof(CompressedFrontable);

        persist_write_data(key, &frontables_to_store[index_offset], size);

        index_offset += num_to_store;
        remaining_frontables -= num_to_store;
    }

    free(frontables_to_store);
}

static void store_groups() {
    CompressedGroup* groups_to_store = malloc(
        sizeof(CompressedGroup) * MAX_CACHED_GROUPS
    );

    // convert cached groups into compressed groups
    uint16_t group_index = 0;
    for (uint16_t i = 0; i < groups.num_stored; i++) {
        if (group_index >= MAX_CACHED_GROUPS) break;

        Group* group = groups.groups[i];

        int16_t parent_index = -1;
        for (uint16_t j = 0; j < groups.num_stored; j++) {
            Group* parent = groups.groups[j];
            if (i != j && group->parent == parent) {
                parent_index = j;
            }
        }

        groups_to_store[group_index] = (CompressedGroup) {
            .color = group->color.argb,
            .parent_index = (uint8_t)(parent_index + 1)
        };
        string_safe_copy(
            groups_to_store[group_index].name,
            group->name,
            COMPRESSED_NAME_LENGTH
        );

        group_index++;
    }

    // store chunks
    int32_t remaining_groups = group_index;
    persist_write_int(GROUPS_NUM_KEY, remaining_groups);
    uint16_t index_offset = 0;
    for (uint32_t key = GROUPS_KEY_MIN; key <= GROUPS_KEY_MAX; key++) {
        if (remaining_groups <= 0) break;

        uint16_t num_to_store = PERSIST_DATA_MAX_LENGTH / sizeof(CompressedGroup);
        if (num_to_store > remaining_groups) num_to_store = remaining_groups;
        uint16_t size = num_to_store * sizeof(CompressedFrontable);

        persist_write_data(key, &groups_to_store[index_offset], size);

        index_offset += num_to_store;
        remaining_groups -= num_to_store;
    }

    free(groups_to_store);
}

static void load_frontables(char* pronoun_map) {
    CompressedFrontable* cached_frontables = (CompressedFrontable*)malloc(
        sizeof(CompressedFrontable) * MAX_CACHED_FRONTABLES
    );
    int32_t num_frontables = persist_read_int(FRONTABLES_NUM_KEY);

    // retrieve chunks from storage
    int32_t remaining_frontables = num_frontables;
    uint16_t index_offset = 0;
    for (uint32_t key = FRONTABLES_KEY_MIN; key <= FRONTABLES_KEY_MAX; key++) {
        if (remaining_frontables <= 0) break;

        uint16_t num_to_load = PERSIST_DATA_MAX_LENGTH / sizeof(CompressedFrontable);
        if (num_to_load > remaining_frontables) num_to_load = remaining_frontables;
        uint16_t size = num_to_load * sizeof(CompressedFrontable);

        persist_read_data(key, &cached_frontables[index_offset], size);

        index_offset += num_to_load;
        remaining_frontables -= num_to_load;
    }

    // load frontables from stored compressed data
    for (int32_t i = 0; i < num_frontables; i++) {
        CompressedFrontable* cached = &cached_frontables[i];

        Frontable* f = frontable_create(
            cached->hash,
            cached->name,
            NULL,
            false,
            GColorBlack
        );

        f->packed_data = cached->packed_data;
        f->group_bit_field = cached->group_bit_field;

        if (cached->pronoun_index > 0) {
            uint16_t index = cached->pronoun_index - 1;
            string_safe_copy(
                f->pronouns,
                &pronoun_map[index * (COMPRESSED_PRONOUNS_LENGTH)],
                FRONTABLE_PRONOUNS_LENGTH
            );
        }

        cache_add_frontable(f);
        if (frontable_get_is_fronting(f)) {
            cache_add_current_fronter(f->hash);
        }
    }

    // re-iterate to assign frontables to groups
    for (uint16_t i = 0; i < members.num_stored; i++) {
        Frontable* member = members.frontables[i];

        for (uint16_t j = 0; j < groups.num_stored; j++) {
            Group* group = groups.groups[j];

            if (((member->group_bit_field >> j) & 1) != 0) {
                frontable_list_add(member, group->frontables);
            }
        }
    }

    free(cached_frontables);
}

static void load_groups() {
    CompressedGroup* cached_groups = malloc(
        sizeof(CompressedGroup) * MAX_CACHED_GROUPS
    );
    int32_t num_groups = persist_read_int(GROUPS_NUM_KEY);

    // retrieve chunks from storage
    int32_t remaining_groups = num_groups;
    uint16_t index_offset = 0;
    for (uint32_t key = GROUPS_KEY_MIN; key <= GROUPS_KEY_MAX; key++) {
        if (remaining_groups <= 0) break;

        uint16_t num_to_load = PERSIST_DATA_MAX_LENGTH / sizeof(CompressedGroup);
        if (num_to_load > remaining_groups) num_to_load = remaining_groups;
        uint16_t size = num_to_load * sizeof(CompressedGroup);

        persist_read_data(key, &cached_groups[index_offset], size);

        index_offset += num_to_load;
        remaining_groups -= num_to_load;
    }

    // load groups from compressed data
    for (int32_t i = 0; i < num_groups; i++) {
        CompressedGroup* cached = &cached_groups[i];

        Group* g = group_create(
            cached->name,
            (GColor) {.argb = cached->color},
            NULL
        );

        cache_add_group(g);
    }

    // assign parent pointers
    for (int32_t i = 0; i < num_groups; i++) {
        CompressedGroup* cached = &cached_groups[i];
        Group* group = groups.groups[i];

        if (cached->parent_index > 0) {
            group->parent = groups.groups[cached->parent_index - 1];
        }
    }

    free(cached_groups);
}

void cache_persist_store() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Attempting to store frontable cache into persistent storage...");

    size_t map_size = sizeof(char) * MAX_CACHED_PRONOUNS * (COMPRESSED_PRONOUNS_LENGTH);
    char* pronoun_map = (char*)malloc(map_size);
    memset(pronoun_map, '\0', map_size);

    store_pronoun_map(pronoun_map, map_size);
    store_frontables(pronoun_map);
    store_groups();

    free(pronoun_map);

    APP_LOG(APP_LOG_LEVEL_INFO, "Frontable cache stored into persistent storage!");
}

bool cache_persist_load() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Attempting to load frontable cache from persistent storage...");

    if (
        !persist_exists(PRONOUNS_KEY) ||
        !persist_exists(FRONTABLES_NUM_KEY) ||
        !persist_exists(GROUPS_NUM_KEY)
    ) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Cannot load persistent data if it was never saved in the first place!");
        return false;
    }

    cache_clear_current_fronters();
    cache_clear_frontables();
    cache_clear_groups();

    // load pronoun map
    size_t size = sizeof(char) * MAX_CACHED_PRONOUNS * (COMPRESSED_PRONOUNS_LENGTH);
    char* pronoun_map = (char*)malloc(size);
    persist_read_data(PRONOUNS_KEY, pronoun_map, size);

    // load groups before frontables, frontables access groups
    load_groups();
    load_frontables(pronoun_map);

    free(pronoun_map);

    APP_LOG(APP_LOG_LEVEL_INFO, "Frontable cache loaded from persistent storage!");

    return true;
}

void cache_persist_delete() {
    persist_delete(PRONOUNS_KEY);
    persist_delete(FRONTABLES_NUM_KEY);
    for (uint32_t key = FRONTABLES_KEY_MIN; key <= FRONTABLES_KEY_MAX; key++) {
        persist_delete(key);
    }
    for (uint32_t key = GROUPS_KEY_MIN; key <= GROUPS_KEY_MAX; key++) {
        persist_delete(key);
    }
}

void frontable_cache_deinit() {
    cache_clear_current_fronters();
    cache_clear_frontables();
    cache_clear_groups();

    if (frontable_queue != NULL) {
        free(frontable_queue);
        frontable_queue = NULL;
    }
    frontable_queue_count = 0;

    if (group_queue != NULL) {
        free(group_queue);
        group_queue = NULL;
    }
    group_queue_count = 0;

    if (current_fronter_queue != NULL) {
        free(current_fronter_queue);
        current_fronter_queue = NULL;
    }
    current_fronter_queue_count = 0;
}
