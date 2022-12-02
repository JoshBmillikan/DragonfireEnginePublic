//
// Created by josh on 12/1/22.
//

#include "local_world.h"
#include "model.h"
#include "transform.h"

namespace df {
LocalWorld::LocalWorld(std::string&& name, ULong seed) : World(std::move(name), seed)
{
    auto entity = registry.create();
    registry.emplace<Model>(entity, "Suzanne", "basic");
    Transform t;
    t.position.y += 2;
    registry.emplace<Transform>(entity, t);
    auto entity2 = registry.create();
    registry.emplace<Model>(entity2, "Suzanne", "basic");
    t.position.y -= 4;
    registry.emplace<Transform>(entity2, t);
}

void LocalWorld::update(double delta)
{
    World::update(delta);
}

LocalWorld::~LocalWorld() noexcept
{
}
}   // namespace df