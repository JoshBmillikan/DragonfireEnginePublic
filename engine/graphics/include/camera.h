//
// Created by josh on 5/25/23.
//

#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace dragonfire {

struct Camera {
    glm::mat4 ortho, perspective;
    glm::vec3 position;
    glm::quat rotation;

    Camera(float fov,
           float width,
           float height,
           glm::vec3 position = glm::vec3(0.0f),
           glm::quat rotation = glm::identity<glm::quat>());
    Camera() = default;
    void lookAt(glm::vec3 target);

    [[nodiscard]] glm::mat4 getViewMatrix() const noexcept
    {
        return glm::translate(glm::identity<glm::mat4>(), position) * glm::toMat4(rotation);
    }
};

}   // namespace dragonfire