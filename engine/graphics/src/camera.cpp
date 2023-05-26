//
// Created by josh on 5/25/23.
//
#include "camera.h"

namespace dragonfire {

Camera::Camera(float fov, float width, float height, glm::vec3 position, glm::quat rotation)
    : ortho(glm::ortho(0.0f, width, 0.0f, height, 0.1f, 1000.0f)),
      perspective(glm::perspective(glm::radians(fov), width / height, 0.1f, 1000.0f)),
      position(position),
      rotation(rotation)
{
    // flip y for compatibility with the Vulkan coordinate system instead of OpenGL
    perspective[1][1] *= -1;
    ortho[1][1] *= -1;
}

void Camera::lookAt(glm::vec3 target)
{
    glm::mat4 look = glm::lookAtRH(position, target, {0, 0, 1});
    position = glm::vec3(look[3]);
    rotation = glm::toQuat(look);
}
}   // namespace dragonfire