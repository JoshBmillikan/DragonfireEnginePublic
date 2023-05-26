//
// Created by josh on 5/25/23.
//

#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace dragonfire {

struct Transform {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    [[nodiscard]] glm::mat4 toMatrix() const
    {
        return glm::translate(glm::mat4(1.0f), position) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
    }

    operator glm::mat4() const { return toMatrix(); }

    Transform& operator+=(const glm::vec3& rhs)
    {
        position += rhs;
        return *this;
    }

    Transform& operator-=(const glm::vec3& rhs)
    {
        position -= rhs;
        return *this;
    }

    Transform operator+(const glm::vec3& rhs) const
    {
        auto result = *this;
        result.position += rhs;
        return result;
    }

    Transform operator-(const glm::vec3& rhs) const
    {
        auto result = *this;
        result.position -= rhs;
        return result;
    }

    Transform& operator*=(const glm::quat& rhs)
    {
        rotation *= rhs;
        return *this;
    }

    Transform& operator+=(const Transform& rhs)
    {
        position += rhs.position;
        rotation *= rhs.rotation;
        scale *= rhs.scale;
        return *this;
    }

    Transform operator+(const Transform& rhs) const
    {
        auto result = *this;
        result.position += rhs.position;
        result.rotation *= rhs.rotation;
        result.scale *= rhs.scale;
        return result;
    }
};
}   // namespace dragonfire