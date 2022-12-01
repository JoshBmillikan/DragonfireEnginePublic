//
// Created by josh on 11/30/22.
//

#include "world.h"
#include "transform.h"

namespace df {

World::World(std::string&& name, ULong seed) : name(std::move(name)), seed(seed), random(seed)
{
}

World* World::newWorld(const char* worldName, ULong worldSeed)
{
    return nullptr;
}

World* World::loadWorld(const char* worldName)
{
    return nullptr;
}

void World::update(double delta)
{
    physics.stepSimulation(delta);
}

World::~World() noexcept
{
    registry.clear<>();
}
}   // namespace df