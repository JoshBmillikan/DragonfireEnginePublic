//
// Created by josh on 5/21/23.
//

#pragma once
#include <glm/glm.hpp>
#include "material.h"

namespace dragonfire {

using MeshHandle = std::uintptr_t;

class Model {
public:
    struct Vertex {
        glm::vec3 position, normal;
        glm::vec2 uv;
    };

    struct Primitive {
        MeshHandle mesh{};
        glm::vec4 bounds;
        Material material{};
        glm::mat4 transform{};
    };

    static Model loadGltfModel(const char* path, class Renderer* renderer, bool optimizeModel = true);

private:
    std::vector<Primitive> primitives;

public:
    [[nodiscard]] const std::vector<Primitive>& getPrimitives() const { return primitives; }
};

}   // namespace dragonfire