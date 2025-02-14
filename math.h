#ifndef CSAND_MATH_H
#define CSAND_MATH_H

#include "x_macros.h"

#define CSAND_GEN_CLAMP(type, name) \
    static inline type csand##name##Clamp(type x, type min, type max) { \
        if (x <= min) { \
            return min; \
        } else if (x <= max) { \
            return x; \
        } else { \
            return max; \
        } \
    }

#define CSAND_GEN_MIN(type, name) \
    static inline type csand##name##Min(type a, type b) { \
        return a <= b ? a : b; \
    }

#define CSAND_GEN_MATH(type, name) \
    CSAND_GEN_CLAMP(type, name) \
    CSAND_GEN_MIN(type, name)

CSAND_X_TYPES(CSAND_GEN_MATH)

#endif
