//
// Created by josh on 11/14/22.
//

#include "mesh.h"

namespace df {

std::array<vk::VertexInputBindingDescription, 2> Mesh::vertexInputDescriptions = {
        vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex),
        vk::VertexInputBindingDescription(1, sizeof(glm::mat4), vk::VertexInputRate::eInstance),
};
std::array<vk::VertexInputAttributeDescription, 7> Mesh::vertexAttributeDescriptions = {
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
        vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)),
        vk::VertexInputAttributeDescription(3, 1, vk::Format::eR32G32B32A32Sfloat, 0),
        vk::VertexInputAttributeDescription(4, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4)),
        vk::VertexInputAttributeDescription(5, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 2),
        vk::VertexInputAttributeDescription(6, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4) * 3),
};
}   // namespace df