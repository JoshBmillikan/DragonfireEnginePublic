//
// Created by josh on 11/9/22.
//

#pragma once

#include "core.h"
#include <cassert>
namespace df {

template<typename T>
class DynamicArray {
    T *start, *bufferEnd;

public:
    DynamicArray() noexcept { start = bufferEnd = nullptr; }
    DynamicArray(Size size)
    {
        start = new T[size];
        bufferEnd = start + size;
    }
    T* begin() noexcept { return start; }
    T* end() noexcept { return bufferEnd; }
    T* data() noexcept { return start; }
    T& operator[](Size index) noexcept
    {
        assert(start + index < bufferEnd);
        return start[index];
    }
};

template<typename T, Size inlineSize>
class InlineDynamicArray {
    T *start, *bufferEnd, array[inlineSize];

public:
    InlineDynamicArray() noexcept { start = bufferEnd = nullptr; }
    InlineDynamicArray(Size size)
    {
        if (size > inlineSize)
            start = new T[size];
        else
            start = array;
        bufferEnd = start + size;
    }
    T* begin() noexcept { return start; }
    T* end() noexcept { return bufferEnd; }
    T* data() noexcept { return start; }
    T& operator[](Size index) noexcept
    {
        assert(start + index < bufferEnd);
        return start[index];
    }
};
}   // namespace df