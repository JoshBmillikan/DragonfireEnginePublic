//
// Created by josh on 11/9/22.
//
#define VMA_IMPLEMENTATION
#include "allocation.h"

namespace df {
VmaAllocator Allocation::allocator;

void Allocation::initAllocator(VmaAllocatorCreateInfo* createInfo)
{
    assert(createInfo);
    if (vmaCreateAllocator(createInfo, &Allocation::allocator) != VK_SUCCESS)
        crash("Failed to initialize VMA allocator");
}
}   // namespace df