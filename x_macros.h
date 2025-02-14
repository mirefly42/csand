#ifndef CSAND_X_MACROS_H
#define CSAND_X_MACROS_H

#define CSAND_X_TYPES(x) \
    x(float, F) \
    x(double, D) \
    x(short, S) \
    x(unsigned short, Us) \
    x(int, I) \
    x(unsigned int, Ui) \
    x(long, L) \
    x(unsigned long, Ul)

#define CSAND_X_OPERATORS(x, ...) \
    x(Add, +, __VA_ARGS__) \
    x(Sub, -, __VA_ARGS__) \
    x(Mul, *, __VA_ARGS__) \
    x(Div, /, __VA_ARGS__)

#endif
