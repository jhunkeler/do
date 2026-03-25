#ifndef DO_TARGET_H
#define DO_TARGET_H

#include <stdlib.h>
#include <sys/stat.h>

#define TARGET_RUN_MAX_DEPTH 100
#define TARGET_SCRIPT_FILENAME_TEMPLATE "do_XXXXXX"

// Data structures
struct TargetInfo {
    size_t line_no;
    char *line;
    char *filename;
};

struct Target {
    char *id;
    char *_tmp_deps;
    char *script;
    struct TargetList *dependencies;
    struct TargetInfo info;
};


// Prototypes
struct Target *target_new();
struct Target *target_clone(const struct Target *src);
char *target_create_script_file(struct Target *t);
int target_run(struct Target *t, const size_t *depth); // single
int target_run_all(struct Target *t, size_t *depth); // recursive
void target_free(struct Target **target);

#endif //DO_TARGET_H