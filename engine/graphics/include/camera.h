//
// Created by josh on 5/25/23.
//

#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace dragonfire {

class Camera {
    glm::mat4 ortho, perspective;
    float zNear;
    float zFar;

public:
    glm::vec3 position;
    glm::quat rotation;

    Camera(float fov,
           float width,
           float height,
           float zNear = 0.1f,
           float zFar = 1000.0f,
           glm::vec3 position = glm::vec3(0.0f),
           glm::quat rotation = glm::identity<glm::quat>());
    Camera() = default;
    void lookAt(glm::vec3 target);

    [[nodiscard]] glm::mat4 getViewMatrix() const noexcept
    {
        return glm::translate(glm::identity<glm::mat4>(), position) * glm::toMat4(rotation);
    }

    [[nodiscard]] const glm::mat4& getOrtho() const { return ortho; }

    [[nodiscard]] const glm::mat4& getPerspective() const { return perspective; }

    [[nodiscard]] std::pair<glm::vec4, glm::vec4> getFrustumPlanes() const;

    [[nodiscard]] float getZNear() const { return zNear; }

    [[nodiscard]] float getZFar() const { return zFar; }
};

}   // namespace dragonfire