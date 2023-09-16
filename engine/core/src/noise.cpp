//
// Created by josh on 6/12/23.
//

#include "noise.h"
#include <glm/glm.hpp>
#include <math_utils.h>

namespace dragonfire {

static glm::vec2 randomGradient(Int x, Int y)
{
    const UInt w = 8 * sizeof(UInt);
    const UInt s = w / 2;   // rotation width
    UInt a = x, b = y;
    a *= 3284157443;
    b ^= a << s | a >> (w - s);
    b *= 1911520717;
    a ^= b << s | b >> (w - s);
    a *= 2048419325;
    float random = float(a) * (3.14159265f / float(~(~0u >> 1)));   // in [0, 2*Pi]
    glm::vec2 v;
    v.x = cosf(random);
    v.y = sinf(random);
    return v;
}

float dotGridGradient(int ix, int iy, float x, float y)
{
    glm::vec2 grad = randomGradient(ix, iy);
    glm::vec2 dis(x - float(ix), y - float(iy));

    return glm::dot(dis, grad);
}

float noise::perlin(float x, float y) noexcept
{
    Int x0 = Int(floorf(x));
    Int x1 = x0 + 1;
    Int y0 = Int(floorf(y));
    Int y1 = y0 + 1;

    float sx = x - (float) x0;
    float sy = y - (float) y0;

    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(x0, y0, x, y);
    n1 = dotGridGradient(x1, y0, x, y);
    ix0 = lerp(n0, n1, sx);

    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix1 = lerp(n0, n1, sx);

    value = lerp(ix0, ix1, sy);
    return value;
}
}   // namespace dragonfire