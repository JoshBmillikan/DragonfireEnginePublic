// This file contains some common definitions
// Created by josh on 11/9/22.
//
#include "glm/vec3.hpp"
#include "spdlog/fmt/bundled/core.h"
#include <ankerl/unordered_dense.h>
#include <cstdint>

#pragma once
namespace df {
template<typename K, typename V>
using HashMap = ankerl::unordered_dense::map<K, V>;

// alias's for fixed width integer types following our naming convention
using UByte = std::uint8_t;
using UShort = std::uint16_t;
using UInt = std::uint32_t;
using ULong = std::uint64_t;
using Byte = std::int8_t;
using Short = std::int16_t;
using Int = std::int32_t;
using Long = std::int64_t;
using IntPtr = std::intptr_t;
using Size = std::size_t;

/// A unit vector in the up direction
constexpr glm::vec3 UP = {0, 0, 1};

constexpr UInt MAX_FILEPATH_LENGTH = 128;

struct FormattedException : public std::runtime_error {
    template<typename... Args>
    FormattedException(fmt::format_string<Args...> msg, Args&&... args) : std::runtime_error(fmt::format(msg, args...))
    {
    }
};
}   // namespace df

#define DF_NO_MOVE(Class)                                                                                                      \
    Class(Class&&) = delete;                                                                                                   \
    Class& operator=(Class&&) = delete;
#define DF_NO_COPY(Class)                                                                                                      \
    Class(const Class&) = delete;                                                                                              \
    Class& operator=(const Class&) = delete;
#define DF_NO_MOVE_COPY(Class)                                                                                                 \
    DF_NO_MOVE(Class)                                                                                                          \
    DF_NO_COPY(Class)

#ifndef APP_NAME
    #define APP_NAME "Test Game"
#endif
