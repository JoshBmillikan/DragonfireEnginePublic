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
    [[nodiscard]] glm::mat4 getViewMatrix() const;
};

}   // namespace df