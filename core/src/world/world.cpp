//
// Created by josh on 11/30/22.
//

#include "world/world.h"
#include "transform.h"

namespace df {

World::World(std::string&& name, ULong seed) : name(std::move(name)), seed(seed), random(seed)
{
    logger = spdlog::default_logger()->clone("World");
    logger->info("Loading world \"{}\"", this->name);
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