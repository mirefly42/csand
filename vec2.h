#ifndef CSAND_VEC2_H
#define CSAND_VEC2_H

#include "math.h"
#include "x_macros.h"

/* WARNING: evaluates the argument twice */
#define CSAND_VEC2_CONVERT(type, vec) \
    ((type){(vec).x, (vec).y})

#define CSAND_VEC2_GEN_OP(name, op, type_name) \
    static inline CsandVec2##type_name csandVec2##type_name##name(CsandVec2##type_name lhs, CsandVec2##type_name rhs) { \
        return (CsandVec2##type_name){lhs.x op rhs.x, lhs.y op rhs.y}; \
    }

#define CSAND_VEC2_GEN_OP_SCALAR(name, op, type_name, type) \
    static inline CsandVec2##type_name csandVec2##type_name##name##Scalar(CsandVec2##type_name lhs, type rhs) { \
        return (CsandVec2##type_name){lhs.x op rhs, lhs.y op rhs}; \
    }

#define CSAND_VEC2_GEN(type, name) \
    typedef struct CsandVec2##name { \
        type x; \
        type y; \
    } CsandVec2##name; \
    \
    static inline CsandVec2##name csandVec2##name##Clamp(CsandVec2##name a, CsandVec2##name min, CsandVec2##name max) { \
        return (CsandVec2##name){csand##name##Clamp(a.x, min.x, max.x), csand##name##Clamp(a.y, min.y, max.y)}; \
    } \
    \
    CSAND_X_OPERATORS(CSAND_VEC2_GEN_OP, name) \
    CSAND_X_OPERATORS(CSAND_VEC2_GEN_OP_SCALAR, name, type) \

CSAND_X_TYPES(CSAND_VEC2_GEN)

#endif
