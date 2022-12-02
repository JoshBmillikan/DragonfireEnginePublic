//
// Created by josh on 11/30/22.
//

#pragma once
#include "entt/entt.hpp"
#include "physics.h"
#include "rng.h"
#include "voxel/chunk.h"

namespace df {

class World {
public:
    const ULong seed;
    virtual ~World() noexcept;
    DF_NO_MOVE_COPY(World);
    virtual void update(double delta);
    Physics& getPhysics() noexcept { return physics; }
    entt::registry& getRegistry() noexcept { return registry; }

protected:
    World(std::string&& name, ULong seed);
    std::shared_ptr<spdlog::logger> logger;
    Physics physics;
    entt::registry registry;
    std::string name;
    voxel::ChunkManager chunkManager;
    Rng random;
};

}   // namespace df