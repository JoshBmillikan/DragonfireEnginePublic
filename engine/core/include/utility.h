//
// Created by josh on 5/18/23.
//

#pragma once
#include <BS_thread_pool.hpp>
#include <nlohmann/json_fwd.hpp>

namespace dragonfire {

nlohmann::json loadJson(const char* path);

inline constexpr USize constStrlen(const char* str) noexcept
{
    if (str == nullptr)
        return 0;
    USize i = 0;
    for (char c = str[0]; c != '\0'; c = *++str)
        i++;
    return i;
}

/// Returns the given size padded to be a multiple of the given alignment
inline constexpr USize padToAlignment(USize s, USize alignment) noexcept
{
    return ((s + alignment - 1) & ~(alignment - 1));
}

inline void hashCombine(USize&)
{
}

template<typename T, typename... Rest>
inline void hashCombine(USize& seed, const T& val, Rest... rest)
{
    std::hash<T> hash;
    seed ^= hash(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hashCombine(seed, rest...);
}

template<typename... Values>
inline USize hashAll(Values... values)
{
    USize seed = 0;
    hashCombine(seed, values...);
    return seed;
}

extern BS::thread_pool GLOBAL_THREAD_POOL;

}   // namespace dragonfire