#ifndef DO_COMMON_H
#define DO_COMMON_H

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"
#include "target.h"
#include "targetlist.h"

#define safe_free(PTR) { \
    do { \
        free((PTR)); \
        (PTR) = NULL; \
    } while (0); \
}

#define safe_free_array(PTR, LEN) { \
    do { \
        for (size_t p = 0; p < (LEN); p++) { free((PTR)[p]); } \
        free((PTR)); \
    } while (0); \
}

#endif //DO_COMMON_H