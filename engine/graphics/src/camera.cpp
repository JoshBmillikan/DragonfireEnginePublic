//
// Created by josh on 5/25/23.
//
#include "camera.h"

namespace dragonfire {

Camera::Camera(float fov, float width, float height, float zNear, float zFar, glm::vec3 position, glm::quat rotation)
    : ortho(glm::ortho(0.0f, width, 0.0f, height, zNear, 1000.0f)),
      perspective(glm::perspective(glm::radians(fov), width / height, zNear, zFar)),
      zNear(zNear),
      zFar(zFar),
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

static glm::vec4 normalizePlane(glm::vec4 p)
{
    return  p / glm::length(glm::vec3(p));
}

std::pair<glm::vec4, glm::vec4> Camera::getFrustumPlanes() const
{
    glm::mat4 pt = glm::transpose(perspective);
    return {
            normalizePlane(pt[3] + pt[0]),
            normalizePlane(pt[3] + pt[1]),
    };
}
}   // namespace dragonfire