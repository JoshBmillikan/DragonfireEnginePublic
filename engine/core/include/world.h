//
// Created by josh on 5/25/23.
//

#pragma once
#include <entt/entt.hpp>
#include "rng.h"

namespace dragonfire {

class World {
public:
    [[nodiscard]] entt::registry& getRegistry() { return registry; }

private:
    entt::registry registry;
    Rng rng;
};

}   // namespace dragonfire