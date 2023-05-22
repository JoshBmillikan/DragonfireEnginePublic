//
// Created by josh on 5/21/23.
//

#pragma once
#include <glm/glm.hpp>

namespace dragonfire {

class Model {
public:
    struct Vertex {
        glm::vec3 position, normal;
        glm::vec2 uv;
    };
};

}   // namespace dragonfire