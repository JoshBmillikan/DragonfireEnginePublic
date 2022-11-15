//
// Created by josh on 11/14/22.
//

#pragma once
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
namespace df {
struct Vertex {
    glm::vec3 position, normal;
    glm::vec2 uv;
};
}   // namespace df