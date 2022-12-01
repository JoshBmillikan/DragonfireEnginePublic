//
// Created by josh on 11/30/22.
//

#pragma once
#include "physics.h"
#include <entt/entt.hpp>
#include <voxel/chunk.h>
#include "rng.h"

namespace df {

class World {
    Physics physics;
    entt::registry registry;
    std::string name;
    voxel::ChunkManager chunkManager;
    Rng random;

    World(std::string&& name, ULong seed);
public:
    const ULong seed;
    ~World() noexcept;
    DF_NO_MOVE_COPY(World);
    void update(double delta);
    Physics& getPhysics() noexcept { return physics; }
    entt::registry& getRegistry() noexcept { return registry; }
    static World* newWorld(const char* worldName, ULong worldSeed);
    static World* newWorld(const std::string& worldName, ULong worldSeed) { return newWorld(worldName.c_str(), worldSeed); }
    static World* loadWorld(const char* worldName);
    static World* loadWorld(const std::string& worldName) { return loadWorld(worldName.c_str()); }
};

}   // namespace df