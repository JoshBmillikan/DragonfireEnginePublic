//
// Created by josh on 6/12/23.
//

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace dragonfire {

struct Vertex {
    glm::vec3 position, normal;
    glm::vec2 uv;
};

}   // namespace dragonfire