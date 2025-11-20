#include "group.h"

#include "../tools/string_tools.h"

Group* group_create(const char* name, GColor color, Group* parent) {
    FrontableList* frontables = frontable_list_create();

    Group* group = malloc(sizeof(Group));
    *group = (Group) {
        .color = color,
        .frontables = frontables,
        .parent = parent
    };
    string_copy_smaller(group->name, name, GROUP_NAME_LENGTH);

    return group;
}

void group_destroy(Group* group) {
    frontable_list_destroy(group->frontables);
    free(group);
}
