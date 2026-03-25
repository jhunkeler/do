#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "str.h"

#define MAX_TARGETS 255

#define FREE_SAFE(PTR) { \
    do { \
        free((PTR)); \
        (PTR) = NULL; \
    } while (0); \
}
#define FREE_ARRAY(PTR, LEN) { \
    do { \
        for (size_t p = 0; p < (LEN); p++) { free((PTR)[p]); } \
        free((PTR)); \
    } while (0); \
}


struct TargetList {
    struct Target **targets;
    size_t count_used;
    size_t count_alloc;
};

struct TargetInfo {
    size_t line_no;
    char *line;
    char *filename;
};

struct Target {
    char *id;
    char *_tmp_deps;
    struct TargetList *dependencies;
    char *script;
    struct TargetInfo info;
};

void free_target(struct Target **target);
void free_target_list(struct TargetList **list);


char *create_target_script_file(struct Target *t) {
    char template[] = "do_XXXXXX";
    if (mkstemps(template, 0) < 0) {
        perror("mkstemps");
        return NULL;
    }
    return strdup(template);
}

struct TargetList *new_target_list(const size_t len) {
    struct TargetList *list = calloc(1, sizeof(struct TargetList));
    if (!list) {
        fprintf(stderr, "unable to allocate memory for list\n");
        return NULL;
    }

    list->count_alloc = len;
    list->targets = calloc(list->count_alloc + 1, sizeof(struct Target *));
    if (!list->targets) {
        fprintf(stderr, "unable to allocate memory for list->targets\n");
        FREE_SAFE(list);
        return NULL;
    }

    return list;
}

int add_target(struct TargetList *list, struct Target *t);


struct Target *new_target() {
    struct Target *t = calloc(1, sizeof(struct Target));
    return t;
}

struct Target *clone_target(const struct Target *src) {
    struct Target *t = new_target();
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
        t->dependencies = new_target_list(src->dependencies->count_alloc);
        if (!t->dependencies) {
            fprintf(stderr, "unable to allocate memory to clone target dependencies\n");
            return NULL;
        }
        for (size_t i = 0; i < src->dependencies->count_used; i++) {
            struct Target *src_target = src->dependencies->targets[i];
            //struct Target *dep = clone_target(src->dependencies->targets[i]);
            add_target(t->dependencies, src_target);
            //free_target(&dep);
            //add_target(t->dependencies, src->dependencies->targets[i]);
        }
    }
    return t;
}

int append_target_list(struct TargetList **dest, const struct TargetList *src) {
    for (size_t i = 0; i < src->count_used; i++) {
        struct Target *t = src->targets[i];
        if (!t) {
            return -1;
        }
        add_target((*dest), t);
    }
    return 0;
}

int add_target(struct TargetList *list, struct Target *t) {
    if (list->count_used > list->count_alloc) {
        fprintf(stderr, "too many items\n");
        return -1;
    }
    list->targets[list->count_used] = t;
    list->count_used++;
    return 0;
}

void free_target(struct Target **target) {
    FREE_SAFE((*target)->id);
    FREE_SAFE((*target)->_tmp_deps);
    FREE_SAFE((*target)->script);
    FREE_SAFE((*target)->info.line);
    FREE_SAFE((*target)->info.filename);
    if ((*target)->dependencies) {
        // the dependencies are pointers to other targets, not allocated on the heap.
        FREE_SAFE((*target)->dependencies->targets);
        FREE_SAFE((*target)->dependencies);
    }
    FREE_SAFE((*target));
}

void free_target_list(struct TargetList **list) {
    if (!list || !*list) {
        return;
    }
    for (size_t i = 0; i < (*list)->count_used; i++) {
        free_target(&(*list)->targets[i]);
    }
    FREE_SAFE((*list)->targets);
    FREE_SAFE(*list);
}

struct Target *get_target(const struct TargetList *list, const char *id) {
    for (size_t i = 0; i < list->count_used; i++) {
        struct Target *t = list->targets[i];
        if (strcmp(t->id, id) == 0) {
            return t;
        }
    }
    return NULL;
}

struct TargetList *get_targets_from_dofile(const char *filename) {
    struct TargetList *list = new_target_list(MAX_TARGETS);
    if (!list) {
        return NULL;
    }

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        free_target_list(&list);
        return NULL;
    }

    char *line = calloc(BUFSIZ + 1, sizeof(char));
    if (!line) {
        free_target_list(&list);
        return NULL;
    }

    size_t buffer_size = BUFSIZ;
    struct Target *t = NULL;
    size_t line_no = 0;
    while (getline(&line, &buffer_size, fp) > 0) {
        // Skip comments
        if (line[0] == '#') {
            line_no++;
            continue;
        }

        char *deps_delim = strchr(line, ':');
        const char *deps = deps_delim ? deps_delim + 1 : NULL;
        char *var_delim = strchr(line, '=');
        const int is_var = isalpha(line[0]) && var_delim != NULL;
        const int is_target = isalnum(line[0]) && deps_delim != NULL;
        const int is_script = isspace(line[0]);
        const int is_include = strncmp(line, "include", strlen("include")) == 0;

        if (t && is_script) {
            if (!t->script) {
                t->script = calloc(BUFSIZ + 1, sizeof(char));
                if (!t->script) {
                    fprintf(stderr, "unable to allocate memory for script data\n");
                    FREE_SAFE(line);
                    free_target_list(&list);
                    return NULL;
                }
                t->info.filename = strdup(filename);
                if (!t->info.filename) {
                    fprintf(stderr, "unable to allocate memory for info.filename\n");
                    FREE_SAFE(line);
                    free_target_list(&list);
                    return NULL;
                }
            }
            char *buf_start = line;
            for (size_t i = 0; i < strlen(line); i++) {
                if (line[i] == '\t' && i < 4) {
                    continue;
                }
                if (isspace(line[i]) && i < 4) {
                    continue;
                }
                buf_start = &line[i];
                break;
            }
            strcat(t->script, buf_start);
        } else if (is_target) {
            t = new_target();
            if (!t) {
                fprintf(stderr, "unable to allocate memory for new target\n");
                FREE_SAFE(line);
                free_target(&t);
                free_target_list(&list);
                return NULL;
            }
            t->info.line_no = line_no;
            t->info.line = strdup(line);

            if (!deps_delim) {
                fprintf(stderr, "%s: line %lu: invalid target definition: %s\n", filename, t->info.line_no, line);
                fprintf(stderr, ">>> %s\n", t->info.line);
                FREE_SAFE(line);
                free_target(&t);
                free_target_list(&list);
                return NULL;
            }
            *deps_delim = '\0';
            t->id = strdup(line);
            if (deps) {
                t->_tmp_deps = strdup(deps);
                if (!t->_tmp_deps) {
                    fprintf(stderr, "unable to allocate memory for temporary dependency data\n");
                    FREE_SAFE(line);
                    free_target(&t);
                    free_target_list(&list);
                    return NULL;
                }
                if (t->_tmp_deps[strlen(t->_tmp_deps) ? strlen(t->_tmp_deps) - 1 : 0] == '\n') {
                    t->_tmp_deps[strlen(t->_tmp_deps) ? strlen(t->_tmp_deps) - 1 : 0] = '\0';
                }
            }
            add_target(list, t);
        } else if (is_var) {
            printf("variable detected\n");
            size_t num_split = 0;
            char **pair = split(line, "=", 1, &num_split);
            if (num_split < 2) {
                fprintf(stderr, "%s: line: %lu: invalid variable definition: %s\n", filename, line_no, line);
                fprintf(stderr, ">>> %s\n", line);
                FREE_SAFE(line);
                free_target(&t);
                free_target_list(&list);
                return NULL;
            }
            FREE_ARRAY(pair, num_split);
        } else if (is_include) {
            size_t num_split = 0;
            char **pair = split(line, " ", 1, &num_split);
            if (!pair) {
                fprintf(stderr, "%s: line: %lu: invalid include definition: %s\n", filename, line_no, line);
                fprintf(stderr, ">>> %s\n", line);
                FREE_SAFE(line);
                free_target(&t);
                free_target_list(&list);
                return NULL;
            }
            if (pair[1][strlen(pair[1]) ? strlen(pair[1]) - 1 : 0] == '\n') {
                pair[1][strlen(pair[1]) ? strlen(pair[1]) - 1 : 0] = '\0';
            }

            printf("Including file: %s\n", pair[1]);
            struct TargetList *included_targets = get_targets_from_dofile(pair[1]);
            if (included_targets) {
                if (append_target_list(&list, included_targets)) {
                    fprintf(stderr, "unable to append to target list %p\n", list);
                    FREE_SAFE(line);
                    free_target_list(&list);
                    return NULL;
                }
                //free_target_list(&included_targets);
                for (size_t i = 0; i < list->count_used; i++) {
                    included_targets->targets[i] = NULL;
                }
                free(included_targets->targets);
                free(included_targets);
            } else {
                fprintf(stderr, "%s: line %lu: failed to include file: %s\n", filename, line_no, pair[1]);
                FREE_SAFE(line);
                FREE_ARRAY(pair, num_split);
                free_target(&t);
                free_target_list(&list);
                return NULL;
            }
            FREE_ARRAY(pair, num_split);
        }
        line_no++;
    }

    // Add dependencies
    for (size_t i = 0; i < list->count_used; i++) {
        if (!list->targets[i]->_tmp_deps || !strlen(list->targets[i]->_tmp_deps)) {
            continue;
        }
        char *_tmp_deps_orig = list->targets[i]->_tmp_deps;
        list->targets[i]->dependencies = new_target_list(MAX_TARGETS);
        const char *token = NULL;
        while ((token = strsep(&list->targets[i]->_tmp_deps, " ")) != NULL) {
            char *data = strdup(token);
            if (!strlen(data)) {
                FREE_SAFE(data);
                continue;
            }

            if (data[strlen(data) ? strlen(data) - 1 : 0] == '\n') {
                data[strlen(data) ? strlen(data) - 1 : 0] = '\0';
            }

            const int target_exists = get_target(list, data) != NULL;
            if (!target_exists) {
                fprintf(stderr, "%s: line %lu: target not found: %s\n", filename, line_no, data);
                fprintf(stderr, ">>> %s\n", list->targets[i]->info.line);
                FREE_SAFE(data);
                FREE_SAFE(line);
                free_target_list(&list);
                return NULL;
            }

            for (size_t j = 0; j < list->count_used; j++) {
                if (strcmp(list->targets[j]->id, data) == 0) {
                    if (list->targets[i]->id == list->targets[j]->id) {
                        fprintf(stderr, "%s: line %lu: target '%s' cannot target itself (circular dependency)\n", filename, list->targets[i]->info.line_no + 1, list->targets[i]->id);
                        fprintf(stderr, ">>> %s\n", list->targets[i]->info.line);
                        free_target(&t);
                        free_target_list(&list);
                        return NULL;
                    }
                    add_target(list->targets[i]->dependencies, list->targets[j]);
                }
            }
            FREE_SAFE(data);
        }
        list->targets[i]->_tmp_deps = _tmp_deps_orig;
    }

    FREE_SAFE(line);
    fclose(fp);
    return list;
}

void show_targets(const struct TargetList *list) {
    for (size_t i = 0; i < list->count_used; i++) {
        const struct Target *t = list->targets[i];
        printf("TARGET: %s\n", t->id);
        printf("DEPENDENCIES: ");
        if (t->dependencies && t->dependencies->count_used) {
            printf("\n");
            for (size_t j = 0; j < t->dependencies->count_used; j++) {
                printf("    - %s\n", t->dependencies->targets[j]->id);
            }
        } else {
            printf("N/A\n");
        }
        printf("SCRIPT: ");
        if (t->script && strlen(t->script)) {
            printf("\n%s\n", t->script);
        } else {
            printf("N/A\n");
        }
        printf("\n");
    }
}

int run_target(struct Target *t, const size_t *depth) {
    if (!t || *depth > 100) {
        return -1;
    }
    char *filename = create_target_script_file(t);
    if (!filename) {
        fprintf(stderr, "failed to create target script\n");
        return -1;
    }

    if (chmod(filename, 0750) < 0) {
        perror("chmod");
        FREE_SAFE(filename);
        return -1;
    }

    FILE *fp = fopen(filename, "w+");
    if (!fp) {
        perror("unable to open target script for writing");
        remove(filename);
        FREE_SAFE(filename);
        return -1;
    }
    fprintf(fp, "%s", t->script ? t->script : "");
    fclose(fp);

    char *cmd = NULL;
    if (asprintf(&cmd, "bash %s", filename) < 0) {
        perror("asprintf");
        remove(filename);
        FREE_SAFE(filename);
        return -1;
    }

    printf("==> Running target %s\n", t->id);
    const int status = system(cmd);
    remove(filename);

    FREE_SAFE(cmd);
    FREE_SAFE(filename);

    return status;
}

#define MAX_DEPTH 100
int run_targets(struct Target *t, size_t *depth) {
    int fail = 0;

    *depth += 1;
    if (*depth > MAX_DEPTH) {
        fprintf(stderr, "maximum recursion depth reached: %d\n", MAX_DEPTH);
        if (t) {
            fprintf(stderr, "%s: line %lu: %s\n", t->info.filename ? t->info.filename : "???", t->info.line_no, t->info.line);
        }
        *depth -= 1;
        return -1;
    }

    if (t && t->dependencies) {
        for (size_t i = 0; i < t->dependencies->count_used; i++) {
            struct Target *d = t->dependencies->targets[i];

            if (run_targets(d, depth)) {
                fail = 1;
                *depth -= 1;
                break;
            }
        }
    }
    if (!fail) {
        *depth -= 1;
        return run_target(t, depth);
    }
    *depth -= 1;
    return -1;
}

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

    struct TargetList *targets = get_targets_from_dofile(dofile_name);
    if (!targets) {
        fprintf(stderr, "%s: failed to resolve all target(s)\n", dofile_name);
        return 1;
    }
    show_targets(targets);
    puts("");

    struct Target *target = get_target(targets, target_name);
    if (!target) {
        fprintf(stderr, "%s: target not found: '%s'\n", dofile_name, target_name);
        free_target_list(&targets);
        return 1;
    }

    size_t depth = 0;
    if (run_targets(target, &depth)) {
        fprintf(stderr, "an error occurred\n");
        free_target_list(&targets);
        return 1;
    }

    free_target_list(&targets);
    return 0;
}