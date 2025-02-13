#ifndef CSAND_RANDOM_H
#define CSAND_RANDOM_H

#include <stdbool.h>
#include <stdint.h>

/* Maps probability from 0-1 float to 0-65535 uint16_t */
#define NPROB(x) ((uint16_t)((x) * 0xFFFF))

static inline uint32_t csandRand(void)
{
    static uint64_t seed = 1;
    seed = 6364136223846793005*seed + 1;
    return seed >> 32;
}

static inline bool csandChance(uint16_t prob) {
    for (;;) {
        uint16_t x = csandRand();
        if (x < 0xFFFF) {
            return x < prob;
        }
    }
}

#endif
