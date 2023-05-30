//
// Created by josh on 5/17/23.
//

#pragma once

namespace dragonfire {

namespace frame_allocator {
    /***
     * @brief Allocates temporary per-frame memory
     * @param size size of the allocation
     * @return ptr to the allocation
     */
    void* alloc(USize size) noexcept;
    /***
     * @brief Clears the per-frame allocator, all of the previous allocations are invalidated by this call
     */
    void nextFrame() noexcept;
    /***
     * @brief Attempt to free a memory block allocated from the per-frame pool.
     * This is only possible if the memory block is the last allocation.
     * If it is not, this is a no-op.
     * @param ptr pointer to the memory block to free
     * @param size size of the memory block
     * @return true if the block was freed, false if it was not a pointer to the last allocation
     */
    bool freeLast(void* ptr, USize size) noexcept;
}   // namespace frame_allocator

template<typename T>
class FrameAllocator {
public:
    using value_type = T;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    FrameAllocator() noexcept = default;

    value_type* allocate(const USize n) const
    {
        USize size = n * sizeof(value_type);
        void* ptr = frame_allocator::alloc(size);
        if (ptr)
            return (value_type*) ptr;
        throw std::bad_alloc();
    }

    void deallocate(value_type* ptr, USize size) const
    {
        frame_allocator::freeLast(ptr, size);
    }

    template<class U>
    FrameAllocator(const FrameAllocator<U>&)
    {
    }

    template<class U>
    FrameAllocator(const FrameAllocator<U>&&)
    {
    }

    template<class U>
    bool operator==(const FrameAllocator<U>&) const
    {
        return true;
    }

    template<class U>
    bool operator!=(const FrameAllocator<U>&) const
    {
        return false;
    }
};

/// std::string using the per-frame allocator
using TempString = std::basic_string<char, std::char_traits<char>, FrameAllocator<char>>;
}   // namespace dragonfire