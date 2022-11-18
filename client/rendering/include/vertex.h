//
// Created by josh on 11/14/22.
//

#pragma once
#include "glm/gtx/hash.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
namespace df {
struct Vertex {
    glm::vec3 position, normal;
    glm::vec2 uv;

    bool operator==(const Vertex& other) const noexcept
    {
        return position == other.position && normal == other.normal && uv == other.uv;
    }
};
}   // namespace df

namespace std {
template<>
struct hash<df::Vertex> {
    size_t operator()(df::Vertex const& vertex) const
    {
        return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1)
               ^ (hash<glm::vec2>()(vertex.uv) << 1);
    }
};
}   // namespace std
