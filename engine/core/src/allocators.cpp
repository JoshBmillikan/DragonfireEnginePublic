//
// Created by josh on 5/17/23.
//

#include "allocators.h"
#include <mutex>
#include <sanitizer/asan_interface.h>

namespace dragonfire {
static constexpr USize MAX_SIZE = 1 << 21;   // 2mb
static char MEMORY[MAX_SIZE];
static USize OFFSET = 0;
static std::mutex MUTEX;

void* frame_allocator::alloc(USize size) noexcept
{
    if (size == 0)
        return nullptr;
    std::unique_lock lock(MUTEX);
    if (OFFSET + size < MAX_SIZE) {
        void* ptr = &MEMORY[OFFSET];
        OFFSET += size;
        ASAN_UNPOISON_MEMORY_REGION(ptr, size);
        SPDLOG_TRACE("Allocated per-frame memory block of size {}", size);
        return ptr;
    }
    spdlog::warn("Per-Frame memory pool exhausted");
    return nullptr;
}

void frame_allocator::nextFrame() noexcept
{
    std::unique_lock lock(MUTEX);
    OFFSET = 0;
    ASAN_POISON_MEMORY_REGION(MEMORY, MAX_SIZE);
}

bool frame_allocator::freeLast(void* ptr, USize size) noexcept
{
    std::unique_lock lock(MUTEX);
    if (OFFSET < size)
        return false;
    void* ptr2 = &MEMORY[OFFSET - size];
    if (ptr == ptr2) {
        OFFSET -= size;
        ASAN_POISON_MEMORY_REGION(ptr2, size);
        SPDLOG_TRACE("Freed per-frame memory block of size {}", size);
        return true;
    }
    return false;
}
}   // namespace dragonfire