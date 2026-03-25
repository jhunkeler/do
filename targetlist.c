#include "common.h"

struct TargetList *target_list_new(const size_t len) {
    struct TargetList *list = calloc(1, sizeof(struct TargetList));
    if (!list) {
        fprintf(stderr, "unable to allocate memory for list\n");
        return NULL;
    }

    list->count_alloc = len;
    list->targets = calloc(list->count_alloc + 1, sizeof(struct Target *));
    if (!list->targets) {
        fprintf(stderr, "unable to allocate memory for list->targets\n");
        safe_free(list);
        return NULL;
    }

    return list;
}

// Append a target list to an existing target list
int target_list_append(struct TargetList **dest, const struct TargetList *src) {
    for (size_t i = 0; i < src->count_used; i++) {
        struct Target *t = src->targets[i];
        if (!t) {
            return -1;
        }
        target_list_add((*dest), t);
    }
    return 0;
}

// Add target to list
int target_list_add(struct TargetList *list, struct Target *t) {
    if (list->count_used >= list->count_alloc) {
        fprintf(stderr, "%p: growing list from %zu records ", list, list->count_alloc);
        list->count_alloc *= 2;
        fprintf(stderr, "to %zu records\n", list->count_alloc);
        struct Target **tmp = realloc(list->targets, list->count_alloc * sizeof(*list->targets));
        if (!tmp) {
            fprintf(stderr, "unable to reallocate memory for target list\n");
            return -1;
        }
        list->targets = tmp;
    }
    list->targets[list->count_used] = t;
    list->count_used++;
    return 0;
}

void target_list_free(struct TargetList **list) {
    if (!list || !*list) {
        return;
    }
    for (size_t i = 0; i < (*list)->count_used; i++) {
        target_free(&(*list)->targets[i]);
    }
    safe_free((*list)->targets);
    safe_free(*list);
}

struct Target *target_list_get(const struct TargetList *list, const char *id) {
    for (size_t i = 0; i < list->count_used; i++) {
        struct Target *t = list->targets[i];
        if (strcmp(t->id, id) == 0) {
            return t;
        }
    }
    return NULL;
}

struct TargetList *target_list_from_file(const char *filename) {
    struct TargetList *list = target_list_new(TARGETLIST_INITIAL_COUNT);
    if (!list) {
        return NULL;
    }

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        target_list_free(&list);
        return NULL;
    }

    char *line = calloc(BUFSIZ + 1, sizeof(char));
    if (!line) {
        target_list_free(&list);
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
                    safe_free(line);
                    target_list_free(&list);
                    return NULL;
                }
                t->info.filename = strdup(filename);
                if (!t->info.filename) {
                    fprintf(stderr, "unable to allocate memory for info.filename\n");
                    safe_free(line);
                    target_list_free(&list);
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
            t = target_new();
            if (!t) {
                fprintf(stderr, "unable to allocate memory for new target\n");
                safe_free(line);
                target_free(&t);
                target_list_free(&list);
                return NULL;
            }
            t->info.line_no = line_no;
            t->info.line = strdup(line);

            if (!deps_delim) {
                fprintf(stderr, "%s: line %lu: invalid target definition: %s\n", filename, t->info.line_no, line);
                fprintf(stderr, ">>> %s\n", t->info.line);
                safe_free(line);
                target_free(&t);
                target_list_free(&list);
                return NULL;
            }
            *deps_delim = '\0';
            t->id = strdup(line);
            if (deps) {
                t->_tmp_deps = strdup(deps);
                if (!t->_tmp_deps) {
                    fprintf(stderr, "unable to allocate memory for temporary dependency data\n");
                    safe_free(line);
                    target_free(&t);
                    target_list_free(&list);
                    return NULL;
                }
                if (t->_tmp_deps[strlen(t->_tmp_deps) ? strlen(t->_tmp_deps) - 1 : 0] == '\n') {
                    t->_tmp_deps[strlen(t->_tmp_deps) ? strlen(t->_tmp_deps) - 1 : 0] = '\0';
                }
            }
            target_list_add(list, t);
        } else if (is_var) {
            printf("variable detected\n");
            size_t num_split = 0;
            char **pair = split(line, "=", 1, &num_split);
            if (num_split < 2) {
                fprintf(stderr, "%s: line: %lu: invalid variable definition: %s\n", filename, line_no, line);
                fprintf(stderr, ">>> %s\n", line);
                safe_free(line);
                target_free(&t);
                target_list_free(&list);
                return NULL;
            }
            safe_free_array(pair, num_split);
        } else if (is_include) {
            size_t num_split = 0;
            char **pair = split(line, " ", 1, &num_split);
            if (!pair) {
                fprintf(stderr, "%s: line: %lu: invalid include definition: %s\n", filename, line_no, line);
                fprintf(stderr, ">>> %s\n", line);
                safe_free(line);
                target_free(&t);
                target_list_free(&list);
                return NULL;
            }
            if (pair[1][strlen(pair[1]) ? strlen(pair[1]) - 1 : 0] == '\n') {
                pair[1][strlen(pair[1]) ? strlen(pair[1]) - 1 : 0] = '\0';
            }

            printf("Including file: %s\n", pair[1]);
            struct TargetList *included_targets = target_list_from_file(pair[1]);
            if (included_targets) {
                if (target_list_append(&list, included_targets)) {
                    fprintf(stderr, "unable to append to target list %p\n", list);
                    safe_free(line);
                    target_list_free(&list);
                    return NULL;
                }
                for (size_t i = 0; i < list->count_used; i++) {
                    included_targets->targets[i] = NULL;
                }
                free(included_targets->targets);
                free(included_targets);
            } else {
                fprintf(stderr, "%s: line %lu: failed to include file: %s\n", filename, line_no, pair[1]);
                safe_free(line);
                safe_free_array(pair, num_split);
                target_free(&t);
                target_list_free(&list);
                return NULL;
            }
            safe_free_array(pair, num_split);
        }
        line_no++;
    }

    // Add dependencies
    for (size_t i = 0; i < list->count_used; i++) {
        if (!list->targets[i]->_tmp_deps || !strlen(list->targets[i]->_tmp_deps)) {
            continue;
        }
        char *_tmp_deps_orig = list->targets[i]->_tmp_deps;
        list->targets[i]->dependencies = target_list_new(TARGETLIST_INITIAL_COUNT);
        const char *token = NULL;
        while ((token = strsep(&list->targets[i]->_tmp_deps, " ")) != NULL) {
            char *data = strdup(token);
            if (!strlen(data)) {
                safe_free(data);
                continue;
            }

            if (data[strlen(data) ? strlen(data) - 1 : 0] == '\n') {
                data[strlen(data) ? strlen(data) - 1 : 0] = '\0';
            }

            const int target_exists = target_list_get(list, data) != NULL;
            if (!target_exists) {
                fprintf(stderr, "%s: line %lu: target not found: %s\n", filename, line_no, data);
                fprintf(stderr, ">>> %s\n", list->targets[i]->info.line);
                safe_free(data);
                safe_free(line);
                target_list_free(&list);
                return NULL;
            }

            for (size_t j = 0; j < list->count_used; j++) {
                if (strcmp(list->targets[j]->id, data) == 0) {
                    if (list->targets[i]->id == list->targets[j]->id) {
                        fprintf(stderr, "%s: line %lu: target '%s' cannot target itself (circular dependency)\n", filename, list->targets[i]->info.line_no + 1, list->targets[i]->id);
                        fprintf(stderr, ">>> %s\n", list->targets[i]->info.line);
                        target_free(&t);
                        target_list_free(&list);
                        return NULL;
                    }
                    target_list_add(list->targets[i]->dependencies, list->targets[j]);
                }
            }
            safe_free(data);
        }
        list->targets[i]->_tmp_deps = _tmp_deps_orig;
    }

    safe_free(line);
    fclose(fp);
    return list;
}

void target_list_show(const struct TargetList *list) {
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

