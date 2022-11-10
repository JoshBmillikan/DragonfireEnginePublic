//
// Created by josh on 11/9/22.
//

#pragma once

#include "vulkan_includes.h"

namespace df {

class Allocation {
public:
    static void initAllocator(VmaAllocatorCreateInfo* createInfo);
    static void shutdownAllocator() {
        vmaDestroyAllocator(allocator);
    }
protected:
    VmaAllocation allocation;
    VmaAllocationInfo info;
    static VmaAllocator allocator;
};

}   // namespace df