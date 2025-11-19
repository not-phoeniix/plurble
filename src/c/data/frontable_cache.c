#include "frontable_cache.h"
#include "../menus/main_menu.h"
#include "../tools/string_tools.h"

#define PRONOUNS_KEY 2
#define FRONTABLES_NUM_KEY 3
#define FRONTABLES_KEY_MIN 4
#define FRONTABLES_KEY_MAX 20

// tweak these to adjust how much memory is allocated
#define MAX_CACHED_FRONTABLES 128
#define MAX_CACHED_PRONOUNS 22
#define COMPRESSED_NAME_LENGTH 19
#define COMPRESSED_PRONOUNS_LENGTH 10

// ~~~ current memory footprint ~~~
//
// max memory taken up by pronoun map:
//   MAX_CACHED_PRONOUNS * (COMPRESSED_PRONOUNS_LENGTH + 1) == 242 bytes
//
// max memory taken up by frontables:
//   sizeof(CompressedFrontable) * MAX_CACHED_FRONTABLES == 3584 bytes
//   ^ this should equate to 14 chunks stored
//
// total memory footprint: 3826 bytes

typedef struct {
    char name[COMPRESSED_NAME_LENGTH + 1];
    uint32_t hash;
    int16_t pronoun_index;
    uint8_t packed_data;
} CompressedFrontable;

static FrontableList members;
static FrontableList custom_fronts;
static FrontableList current_fronters;
static GroupCollection groups;

FrontableList* cache_get_members() { return &members; }
FrontableList* cache_get_custom_fronts() { return &custom_fronts; }
FrontableList* cache_get_current_fronters() { return &current_fronters; }
Frontable* cache_get_first_fronter() {
    if (current_fronters.frontables != NULL) {
        return current_fronters.frontables[0];
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
    Frontable* frontable = NULL;

    //! using just iteration to search isn't the best idea...
    //!   for the future, since we have hashes for each frontable:
    //!   make a hash map maybe !!!!

    for (uint16_t i = 0; i < members.num_stored; i++) {
        Frontable* f = members.frontables[i];

        if (f->hash == frontable_hash) {
            frontable = f;
            break;
        }
    }

    if (frontable == NULL) {
        for (uint16_t i = 0; i < custom_fronts.num_stored; i++) {
            Frontable* f = custom_fronts.frontables[i];

            if (f->hash == frontable_hash) {
                frontable = f;
                break;
            }
        }
    }

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
        APP_LOG(APP_LOG_LEVEL_ERROR, "ERROR! Cannot add any more groups to cache, limit has been reached!");
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
        free(groups.groups[i]);
        groups.groups[i] = NULL;
    }
    groups.num_stored = 0;
}

static void store_pronoun_map(char* pronoun_map, size_t pronoun_map_size) {
    uint16_t pronoun_index = 0;

    for (uint16_t i = 0; i < members.num_stored; i++) {
        Frontable* member = members.frontables[i];

        bool pronouns_exist = false;
        for (uint16_t j = 0; j < MAX_CACHED_PRONOUNS; j++) {
            char* pronouns = pronoun_map + (j * (COMPRESSED_PRONOUNS_LENGTH + 1));
            if (string_start_same(member->pronouns, pronouns)) {
                pronouns_exist = true;
                break;
            }
        }

        // add to array if they haven't been cached yet
        if (!pronouns_exist && pronoun_index < MAX_CACHED_PRONOUNS) {
            string_copy_smaller(
                pronoun_map + (pronoun_index * (COMPRESSED_PRONOUNS_LENGTH + 1)),
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

    for (uint16_t i = 0; i < custom_fronts.num_stored; i++) {
        if (frontable_index >= MAX_CACHED_FRONTABLES) break;

        Frontable* custom_front = custom_fronts.frontables[i];

        frontables_to_store[frontable_index] = (CompressedFrontable) {
            .hash = custom_front->hash,
            .packed_data = custom_front->packed_data,
            .pronoun_index = -1
        };
        string_copy_smaller(
            frontables_to_store[frontable_index].name,
            custom_front->name,
            COMPRESSED_NAME_LENGTH
        );

        frontable_index++;
    }

    for (uint16_t i = 0; i < members.num_stored; i++) {
        if (frontable_index >= MAX_CACHED_FRONTABLES) break;

        Frontable* member = members.frontables[i];

        // find index of pronouns, pronoun_index counts the
        //   number of stored pronouns at this point
        int16_t pronoun_index = -1;
        for (uint16_t j = 0; j < MAX_CACHED_PRONOUNS; j++) {
            if (string_start_same(member->pronouns, pronoun_map + (j * (COMPRESSED_PRONOUNS_LENGTH + 1)))) {
                pronoun_index = j;
                break;
            }
        }

        frontables_to_store[frontable_index] = (CompressedFrontable) {
            .hash = member->hash,
            .packed_data = member->packed_data,
            .pronoun_index = pronoun_index
        };
        string_copy_smaller(
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

        persist_write_data(key, frontables_to_store + index_offset, size);

        index_offset += num_to_store;
        remaining_frontables -= num_to_store;
    }

    free(frontables_to_store);
}

void cache_persist_store() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Attempting to store frontable cache into persistent storage...");

    size_t map_size = sizeof(char) * MAX_CACHED_PRONOUNS * (COMPRESSED_PRONOUNS_LENGTH + 1);
    char* pronoun_map = (char*)malloc(map_size);
    memset(pronoun_map, '\0', map_size);

    store_pronoun_map(pronoun_map, map_size);
    store_frontables(pronoun_map);

    free(pronoun_map);

    APP_LOG(APP_LOG_LEVEL_INFO, "Frontable cache stored into persistent storage!");
}

bool cache_persist_load() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Attempting to load frontable cache from persistent storage...");

    if (
        !persist_exists(PRONOUNS_KEY) ||
        !persist_exists(FRONTABLES_NUM_KEY) ||
        !persist_exists(FRONTABLES_KEY_MIN)
    ) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Cannot load persistent data if it was never saved in the first place!");
        return false;
    }

    cache_clear_current_fronters();
    cache_clear_frontables();

    size_t size = sizeof(char) * MAX_CACHED_PRONOUNS * (COMPRESSED_PRONOUNS_LENGTH + 1);
    char* pronoun_map = (char*)malloc(size);
    persist_read_data(PRONOUNS_KEY, pronoun_map, size);

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

        persist_read_data(key, cached_frontables + index_offset, size);

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

        if (cached->pronoun_index >= 0) {
            string_copy_smaller(
                f->pronouns,
                // pronoun_map + (cached->pronoun_index * (COMPRESSED_PRONOUNS_LENGTH + 1)),
                pronoun_map + (cached->pronoun_index * (COMPRESSED_PRONOUNS_LENGTH + 1)),
                FRONTABLE_PRONOUNS_LENGTH
            );
        }

        cache_add_frontable(f);
        if (frontable_get_is_fronting(f)) {
            cache_add_current_fronter(f->hash);
        }
    }

    // free previous memory
    free(pronoun_map);
    free(cached_frontables);

    APP_LOG(APP_LOG_LEVEL_INFO, "Frontable cache loaded from persistent storage!");

    return true;
}

void cache_persist_delete() {
    persist_delete(PRONOUNS_KEY);
    persist_delete(FRONTABLES_NUM_KEY);
    for (uint32_t key = FRONTABLES_KEY_MIN; key <= FRONTABLES_KEY_MAX; key++) {
        persist_delete(key);
    }
}

void frontable_cache_deinit() {
    cache_clear_current_fronters();
    cache_clear_frontables();
}
