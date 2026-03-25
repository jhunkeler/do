#ifndef DO_TARGETLIST_H
#define DO_TARGETLIST_H

#include "target.h"

#define TARGETLIST_INITIAL_COUNT 100

// Data structures
struct TargetList {
    size_t count_used;
    size_t count_alloc;
    struct Target **targets;
};

// Prototypes
struct TargetList *target_list_new(size_t len);
struct TargetList *target_list_from_file(const char *filename);
int target_list_add(struct TargetList *list, struct Target *t);
int target_list_append(struct TargetList **dest, const struct TargetList *src);
struct Target *target_list_get(const struct TargetList *list, const char *id);
void target_list_show(const struct TargetList *list);
void target_list_free(struct TargetList **list);

#endif //DO_TARGETLIST_H