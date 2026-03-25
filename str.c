//
// Created by jhunk on 3/21/26.
//

#include "str.h"

int num_chars(const char *sptr, int ch) {
    int result = 0;
    for (int i = 0; sptr[i] != '\0'; i++) {
        if (sptr[i] == ch) {
            result++;
        }
    }
    return result;
}

char** split(char *_sptr, const char* delim, size_t max, size_t *num)
{
    // Zero the number of elements in the result
    int have_num = num != NULL;
    if (have_num) {
        *num = 0;
    }

    if (_sptr == NULL || delim == NULL) {
        return NULL;
    }
    size_t split_alloc = 0;
    // Duplicate the input string and save a copy of the pointer to be freed later
    char *orig = _sptr;
    char *sptr = strdup(orig);

    if (!sptr) {
        return NULL;
    }

    // Determine how many delimiters are present
    for (size_t i = 0; i < strlen(delim); i++) {
        if (max && i > max) {
            break;
        }
        split_alloc += num_chars(sptr, delim[i]);
    }

    // Preallocate enough records based on the number of delimiters
    char **result = calloc(split_alloc + 2, sizeof(result[0]));
    if (!result) {
        free(sptr);
        sptr = NULL;
        return NULL;
    }

    // No delimiter, but the string was not NULL, so return the original string
    if (split_alloc == 0) {
        result[0] = sptr;
        return result;
    }

    // Separate the string into individual parts and store them in the result array
    char *token = NULL;
    char *sptr_tmp = sptr;
    size_t pos = 0;
    size_t i;
    for (i = 0; (token = strsep(&sptr_tmp, delim)) != NULL; i++) {
        // When max is zero, record all tokens
        if (max > 0 && i == max) {
            // Maximum number of splits occurred.
            // Record position in string
            pos = token - sptr;
            break;
        }
        result[i] = calloc(BUFSIZ, sizeof(char));
        if (!result[i]) {
            return NULL;
        }
        strcpy(result[i], token);
    }

    // pos is non-zero when maximum split is reached
    if (pos) {
        // append the remaining string contents to array
        result[i] = calloc(BUFSIZ, sizeof(char));
        if (!result[i]) {
            return NULL;
        }
        strcpy(result[i], &orig[pos]);
        i++;
    }

    if (have_num) {
        // Count of elements in the array
        *num = i;
    }

    free(sptr);
    sptr = NULL;
    return result;
}

