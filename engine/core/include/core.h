//
// Created by josh on 5/17/23.
//

#pragma once
#include <cstdint>
#include <fmt/format.h>
#include <source_location>
#include <stdexcept>

namespace dragonfire {

using UInt8 = std::uint8_t;
using UInt16 = std::uint16_t;
using UInt32 = std::uint32_t;
using UInt64 = std::uint64_t;
using UInt = UInt32;
using Int8 = std::int8_t;
using Int16 = std::int16_t;
using Int32 = std::int32_t;
using Int64 = std::int64_t;
using Int = Int32;
using USize = std::size_t;

#ifdef NDEBUG
[[noreturn]] void crash(const char* msg);

template<typename... Args>
[[noreturn]] inline void crash(fmt::format_string<Args...> msg, Args&&... args)
{
    crash(fmt::format(msg, std::forward<Args>(args)...).c_str());
}

inline void require(bool condition)
{
    if (!condition)
        crash("Required condition was invalid");
}

#else

[[noreturn]] void crash(
        const char* msg,
        const std::source_location& location = std::source_location::current()
);

template<typename T1>
[[noreturn]] inline void crash(
        fmt::format_string<T1> msg,
        T1 a,
        std::source_location location = std::source_location::current()
)
{
    crash(fmt::format(msg, std::forward<T1>(a)).c_str(), location);
}

template<typename T1, typename T2>
    requires(!std::is_same_v<T2, std::source_location>)
[[noreturn]] inline void crash(
        fmt::format_string<T1, T2> msg,
        T1 a,
        T2 b,
        std::source_location location = std::source_location::current()
)
{
    crash(fmt::format(msg, std::forward<T1>(a), std::forward<T2>(b)).c_str(), location);
}

template<typename T1, typename T2, typename T3>
    requires(!std::is_same_v<T3, std::source_location>)
[[noreturn]] inline void crash(
        fmt::format_string<T1, T2, T3> msg,
        T1 a,
        T2 b,
        T3 c,
        std::source_location location = std::source_location::current()
)
{
    crash(fmt::format(msg, std::forward<T1>(a), std::forward<T2>(b), std::forward<T3>(c)).c_str(),
          location);
}

template<typename T1, typename T2, typename T3, typename T4>
    requires(!std::is_same_v<T4, std::source_location>)
[[noreturn]] inline void crash(
        fmt::format_string<T1, T2, T3, T4> msg,
        T1 a,
        T2 b,
        T3 c,
        T4 d,
        std::source_location location = std::source_location::current()
)
{
    crash(fmt::format(
                  msg,
                  std::forward<T1>(a),
                  std::forward<T2>(b),
                  std::forward<T3>(c),
                  std::forward<T4>(d)
          )
                  .c_str(),
          location);
}

inline void require(bool condition, std::source_location location = std::source_location::current())
{
    if (!condition)
        crash("Required condition was invalid", location);
}

#endif

inline void check(bool condition)
{
    if (!condition)
        throw std::runtime_error("Condition check failed");
}

template<typename T, typename F, typename... Args>
inline T exceptionBarrier(F&& func, Args... args)
{
    try {
        return func(std::forward<Args...>(args...));
    }
    catch (const std::exception& e) {
        crash("Unhanded exception: {}", e.what());
    }
    catch (...) {
        crash("Unknown exception encountered");
    }
}

template<typename T, typename F>
inline T exceptionBarrier(F&& func)
{
    try {
        return func();
    }
    catch (const std::exception& e) {
        crash("Unhanded exception: {}", e.what());
    }
    catch (...) {
        crash("Unknown exception encountered");
    }
}

struct FormattedError : public std::runtime_error {
    template<typename... Args>
    FormattedError(fmt::format_string<Args...> msg, Args&&... args)
        : std::runtime_error(fmt::format(msg, std::forward<Args>(args)...))
    {
    }
};

}   // namespace dragonfire