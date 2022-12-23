//
// Created by josh on 11/18/22.
//

#pragma once
#include <glm/gtx/hash.hpp>
#include <glm/gtx/quaternion.hpp>
namespace df {

class Transform {
public:
    glm::vec3 position = glm::vec3(0.0f), scale = glm::vec3(1.0f);
    glm::quat rotation = glm::identity<glm::quat>();
    Transform() = default;
    Transform(glm::vec3 pos, glm::quat rotation = glm::identity<glm::quat>(), glm::vec3 scale = glm::vec3(1.0f))
        : position(pos), scale(scale), rotation(rotation)
    {
    }

    bool operator==(const Transform& other) const noexcept
    {
        return position == other.position && scale == other.scale && rotation == other.rotation;
    }

    [[nodiscard]] glm::mat4 toMatrix() const noexcept
    {
        return glm::scale(glm::translate(glm::identity<glm::mat4>(), position) * glm::toMat4(rotation), scale);
    }

    operator glm::mat4() const noexcept { return toMatrix(); }
};
}   // namespace df

namespace std {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCInconsistentNamingInspection"
template<>
struct [[maybe_unused]] hash<df::Transform> {
    size_t operator()(df::Transform const& transform) const
    {
        return ((hash<glm::vec3>()(transform.position) ^ (hash<glm::vec3>()(transform.scale) << 1)) >> 1)
               ^ (hash<glm::quat>()(transform.rotation) << 1);
    }
};
#pragma clang diagnostic pop
}   // namespace std