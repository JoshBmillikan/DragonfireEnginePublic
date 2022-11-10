//
// Created by josh on 11/9/22.
//
#include <source_location>
#include <spdlog/spdlog.h>

#pragma once
namespace df {

#ifndef ERROR_INCLUDE_SOURCE_LOCATION
    #ifndef NDEBUG
        #define ERROR_INCLUDE_SOURCE_LOCATION 1
    #endif
#endif

#ifdef ERROR_INCLUDE_SOURCE_LOCATION
// C++ currently doesn't allow for variadic function templates with default arguments,
// so we have to declare multiple function overloads for each number of arguments manually.

/// Logs a message at the critical level, then terminates the program.
[[noreturn]] inline void crash(const char* msg, std::source_location location = std::source_location::current())
{
    spdlog::critical(
            R"(Unrecoverable error "{}" in function "{}" at {}:{}:{})",
            msg,
            location.function_name(),
            location.file_name(),
            location.line(),
            location.column()
    );
    exit(-1);
}

/// Logs a message at the critical level, then terminates the program.
template<typename T>
[[noreturn]] inline void crash(fmt::format_string<T> msg, T&& arg, std::source_location location = std::source_location::current())
{
    spdlog::critical(
            R"(Unrecoverable error "{}" in function "{}" at {}:{}:{})",
            fmt::format(msg, std::forward<T>(arg)),
            location.function_name(),
            location.file_name(),
            location.line(),
            location.column()
    );
    exit(-1);
}

/// Logs a message at the critical level, then terminates the program.
template<typename T1, typename T2>
[[noreturn]] inline void crash(
        fmt::format_string<T1, T2> msg,
        T1&& arg1,
        T2&& arg2,
        std::source_location location = std::source_location::current()
)
{
    spdlog::critical(
            R"(Unrecoverable error "{}" in function "{}" at {}:{}:{})",
            fmt::format(msg, std::forward<T1, T2>(arg1, arg2)),
            location.function_name(),
            location.file_name(),
            location.line(),
            location.column()
    );
    exit(-1);
}

/// Logs a message at the critical level, then terminates the program.
template<typename T1, typename T2, typename T3>
[[noreturn]] inline void crash(
        fmt::format_string<T1, T2, T3> msg,
        T1&& arg1,
        T2&& arg2,
        T3&& arg3,
        std::source_location location = std::source_location::current()
)
{
    spdlog::critical(
            R"(Unrecoverable error "{}" in function "{}" at {}:{}:{})",
            fmt::format(msg, std::forward<T1, T2, T3>(arg1, arg2, arg3)),
            location.function_name(),
            location.file_name(),
            location.line(),
            location.column()
    );
    exit(-1);
}

/// Logs a message at the critical level, then terminates the program.
template<typename T1, typename T2, typename T3, typename T4>
[[noreturn]] inline void crash(
        fmt::format_string<T1, T2, T3, T4> msg,
        T1&& arg1,
        T2&& arg2,
        T3&& arg3,
        T4&& arg4,
        std::source_location location = std::source_location::current()
)
{
    spdlog::critical(
            R"(Unrecoverable error "{}" in function "{}" at {}:{}:{})",
            fmt::format(msg, std::forward<T1, T2, T3, T4>(arg1, arg2, arg3, arg4)),
            location.function_name(),
            location.file_name(),
            location.line(),
            location.column()
    );
    exit(-1);
}

/// Logs a message at the critical level, then terminates the program.
template<typename T1, typename T2, typename T3, typename T4, typename T5>
[[noreturn]] inline void crash(
        fmt::format_string<T1, T2, T3, T4, T5> msg,
        T1&& arg1,
        T2&& arg2,
        T3&& arg3,
        T4&& arg4,
        T5&& arg5,
        std::source_location location = std::source_location::current()
)
{
    spdlog::critical(
            R"(Unrecoverable error "{}" in function "{}" at {}:{}:{})",
            fmt::format(msg, std::forward<T1, T2, T3, T4, T5>(arg1, arg2, arg3, arg4, arg5)),
            location.function_name(),
            location.file_name(),
            location.line(),
            location.column()
    );
    exit(-1);
}
#else
/// Logs a message at the critical level, then terminates the program.
template<typename... Args>
[[noreturn]] inline void crash(spdlog::format_string_t<Args...> fmt, Args&&... args)
{
    spdlog::critical(fmt, std::forward<Args>(args)...);
    exit(-1);
}
#endif

#undef ERROR_INCLUDE_SOURCE_LOCATION
}   // namespace df