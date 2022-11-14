//
// Created by josh on 11/9/22.
//
#include <cstdint>

#pragma once
namespace df {
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
