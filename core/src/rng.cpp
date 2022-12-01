//
// Created by josh on 11/30/22.
//

#include "rng.h"

namespace df {
Rng::Rng(ULong seed)
{
    for (ULong& s : state) {
        ULong z = (seed += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        s = z ^ (z >> 31);
    }
}

static inline ULong rotl(const ULong x, Int k)
{
    return (x << k) | (x >> (64 - k));
}

double Rng::nextDouble()
{
    const ULong result = state[0] + state[3];

    const ULong t = state[1] << 17;

    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];

    state[2] ^= t;

    state[3] = rotl(state[3], 45);

    return std::bit_cast<double>(result >> 53);
}

ULong Rng::nextULong()
{
    const ULong result = rotl(state[1] * 5, 7) * 9;

    const ULong t = state[1] << 17;

    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];

    state[2] ^= t;

    state[3] = rotl(state[3], 45);

    return result;
}
}   // namespace df