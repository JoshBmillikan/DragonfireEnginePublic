//
// Created by josh on 5/21/23.
//

#pragma once
#include <glm/glm.hpp>

namespace dragonfire {

using MeshHandle = std::uintptr_t;

class Model {
public:
    struct Vertex {
        glm::vec3 position, normal;
        glm::vec2 uv;
    };
};

}   // namespace dragonfire