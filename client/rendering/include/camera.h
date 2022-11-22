//
// Created by josh on 11/13/22.
//

#pragma once
#include <glm/gtx/quaternion.hpp>
namespace df {

class Camera {
public:
    glm::mat4 perspective, orthographic;
    glm::quat rotation;
    glm::vec3 position;
    [[nodiscard]] glm::mat4 getViewMatrix() const noexcept
    {
        return glm::translate(glm::mat4(1.0f), position) * glm::toMat4(rotation);
    }
    Camera(float fov, UInt width, UInt height, glm::vec3 position = {}, glm::quat rotation = {})
        : perspective(glm::perspective(fov,(float) width /  (float) height, 0.1f, 1000.0f)),
          orthographic(glm::ortho(0.0f, (float) width, 0.0f, (float) height, 0.1f, 1000.0f)),
          position(position),
          rotation(rotation)
    {
        // GLM was designed for opengl, so we must flip the y coordinate to render correctly with vulkan
        perspective[1][1] *= -1;
        orthographic[1][1] *= -1;
    }
    Camera() = default;
    void lookAt(glm::vec3 target) {
        glm::mat4 look = glm::lookAt(position, target, glm::vec3(0.0f, 0.0f, 1.0f));
        position = glm::vec3(look[3]);
        rotation = glm::toQuat(look);
    }
};

}   // namespace df