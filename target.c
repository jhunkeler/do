#include "common.h"

struct Target *target_new() {
    struct Target *t = calloc(1, sizeof(struct Target));
    return t;
}

struct Target *target_clone(const struct Target *src) {
    struct Target *t = target_new();
    if (!t) {
        fprintf(stderr, "unable to allocate memory to clone target\n");
        return NULL;
    }
    t->id = src->id ? strdup(src->id) : NULL;
    t->_tmp_deps = src->_tmp_deps ? strdup(src->_tmp_deps) : NULL;
    t->script = src->script ? strdup(src->script) : NULL;
    t->info.line = src->info.line ? strdup(src->info.line) : NULL;
    t->info.filename = src->info.filename ? strdup(src->info.filename) : NULL;
    if (src->dependencies && src->dependencies->count_used) {
        t->dependencies = target_list_new(src->dependencies->count_alloc);
        if (!t->dependencies) {
            fprintf(stderr, "unable to allocate memory to clone target dependencies\n");
            return NULL;
        }
        for (size_t i = 0; i < src->dependencies->count_used; i++) {
            struct Target *src_target = src->dependencies->targets[i];
            target_list_add(t->dependencies, src_target);
        }
    }
    return t;
}

char *target_create_script_file(struct Target *t) {
    char template[] = TARGET_SCRIPT_FILENAME_TEMPLATE;
    if (mkstemps(template, 0) < 0) {
        perror("mkstemps");
        return NULL;
    }
    return strdup(template);
}

int target_run_all(struct Target *t, size_t *depth) {
    int fail = 0;

    *depth += 1;
    if (*depth > TARGET_RUN_MAX_DEPTH) {
        fprintf(stderr, "maximum recursion depth reached: %d\n", TARGET_RUN_MAX_DEPTH);
        if (t) {
            fprintf(stderr, "%s: line %lu: %s\n", t->info.filename ? t->info.filename : "???", t->info.line_no, t->info.line);
        }
        *depth -= 1;
        return -1;
    }

    if (t && t->dependencies) {
        for (size_t i = 0; i < t->dependencies->count_used; i++) {
            struct Target *d = t->dependencies->targets[i];

            if (target_run_all(d, depth)) {
                fail = 1;
                *depth -= 1;
                break;
            }
        }
    }
    if (!fail) {
        *depth -= 1;
        return target_run(t, depth);
    }
    *depth -= 1;
    return -1;
}

int target_run(struct Target *t, const size_t *depth) {
    if (!t || *depth > 100) {
        return -1;
    }
    char *filename = target_create_script_file(t);
    if (!filename) {
        fprintf(stderr, "failed to create target script\n");
        return -1;
    }

    if (chmod(filename, 0750) < 0) {
        perror("chmod");
        safe_free(filename);
        return -1;
    }

    FILE *fp = fopen(filename, "w+");
    if (!fp) {
        perror("unable to open target script for writing");
        remove(filename);
        safe_free(filename);
        return -1;
    }
    fprintf(fp, "%s", t->script ? t->script : "");
    fclose(fp);

    char *cmd = NULL;
    if (asprintf(&cmd, "bash %s", filename) < 0) {
        perror("asprintf");
        remove(filename);
        safe_free(filename);
        return -1;
    }

    printf("==> Running target %s\n", t->id);
    const int status = system(cmd);
    remove(filename);

    safe_free(cmd);
    safe_free(filename);

    return status;
}

void target_free(struct Target **target) {
    safe_free((*target)->id);
    safe_free((*target)->_tmp_deps);
    safe_free((*target)->script);
    safe_free((*target)->info.line);
    safe_free((*target)->info.filename);
    if ((*target)->dependencies) {
        // Dependencies are shared between lists. Do not wipe out the targets _here_.
        safe_free((*target)->dependencies->targets);
        safe_free((*target)->dependencies);
    }
    safe_free((*target));
}

