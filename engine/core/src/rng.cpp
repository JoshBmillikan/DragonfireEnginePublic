//
// Created by josh on 5/31/23.
// Random number generators based on xoshiro256
// https://prng.di.unimi.it/

#include "rng.h"
#include <SDL_timer.h>

namespace dragonfire {

Rng::Rng(UInt64 seed)
{
    for (auto& state : s) {
        UInt64 z = (seed += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        state = z ^ (z >> 31);
    }
}

Rng::Rng() : Rng(SDL_GetTicks64())
{
}

static inline UInt64 rotl(const UInt64 x, int k)
{
    return (x << k) | (x >> (64 - k));
}

//xoshiro256++
UInt64 Rng::next()
{
    const UInt64 result = rotl(s[0] + s[3], 23) + s[0];
    const UInt64 t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;

    s[3] = rotl(s[3], 45);

    return result;
}

//xoshiro256+
double Rng::nextDouble()
{
    const UInt64 result = s[0] + s[3];

    const UInt64 t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;

    s[3] = rotl(s[3], 45);

    return double(result >> 11) * 0x1.0p-53;
}
}   // namespace dragonfire