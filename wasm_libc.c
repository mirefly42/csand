#include <stddef.h>

void *memcpy(void *restrict dst, const void *restrict src, size_t size) {
    for (size_t i = 0; i < size; i++) {
        ((char *)dst)[i] = ((char *)src)[i];
    }

    return dst;
}

void *memmove(void *dst, const void *src, size_t size) {
    if (src > dst) {
        memcpy(dst, src, size);
    } else if (dst > src) {
        for (size_t i = size; i-- > 0;) {
            ((char *)dst)[i] = ((char *)src)[i];
        }
    }

    return dst;
}

void *memset(void *data, int c, size_t size) {
    for (size_t i = 0; i < size; i++) {
        ((unsigned char *)data)[i] = c;
    }

    return data;
}
