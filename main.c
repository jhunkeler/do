#include "common.h"

int main(int argc, char *argv[]) {
    const char *dofile_name = argv[1] ? argv[1] : "dofile";
    if (!dofile_name || !strlen(dofile_name)) {
        fprintf(stderr, "need a dofile\n");
        return 1;
    }
    const char *target_name = argv[2] ? argv[2] : "default";
    if (!strlen(target_name)) {
        fprintf(stderr, "invalid empty target: '%s'\n", target_name);
        return 1;
    }

    struct TargetList *targets = target_list_from_file(dofile_name);
    if (!targets) {
        fprintf(stderr, "%s: failed to resolve all target(s)\n", dofile_name);
        return 1;
    }
    target_list_show(targets);
    puts("");

    struct Target *target = target_list_get(targets, target_name);
    if (!target) {
        fprintf(stderr, "%s: target not found: '%s'\n", dofile_name, target_name);
        target_list_free(&targets);
        return 1;
    }

    size_t depth = 0;
    if (target_run_all(target, &depth)) {
        fprintf(stderr, "an error occurred\n");
        target_list_free(&targets);
        return 1;
    }

    target_list_free(&targets);
    return 0;
}