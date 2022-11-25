// General math utilities
// Created by josh on 11/25/22.
//

#pragma once
#include <cmath>
#include <glm/glm.hpp>

namespace df {
inline float lerp(float a, float b, float t)
{
    return std::fma(t, b, std::fma(-t, a, a));
}

inline double lerp(double a, double b, double t)
{
    return std::fma(t, b, std::fma(-t, a, a));
}

inline glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t)
{
    return {lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t)};
}
}   // namespace df
