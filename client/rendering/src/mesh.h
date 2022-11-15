//
// Created by josh on 11/14/22.
//

#pragma once
#include "vulkan_includes.h"
#include "vertex.h"
#include "allocation.h"
#include <array>

namespace df {

class Mesh {
    Buffer buffer;
public:
    static std::array<vk::VertexInputBindingDescription, 2> vertexInputDescriptions;
    static std::array<vk::VertexInputAttributeDescription, 7> vertexAttributeDescriptions;
};

}   // namespace df