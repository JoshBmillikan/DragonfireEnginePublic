//
// Created by josh on 5/25/23.
//

#pragma once
#include <entt/entt.hpp>

namespace dragonfire {

class World {
    entt::registry registry;

public:
    [[nodiscard]] entt::registry& getRegistry() { return registry; }
};

}   // namespace dragonfire