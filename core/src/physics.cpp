//
// Created by josh on 11/30/22.
//

#include "physics.h"
#include "game.h"
#include "transform.h"
#include <glm/gtx/matrix_decompose.hpp>

namespace df {
btDefaultCollisionConfiguration Physics::defaultConfig;
btDbvtBroadphase Physics::defaultBroadPhase;
Physics::Physics(glm::vec3 gravity, btCollisionConfiguration* configuration, Int maxSteps)
    : maxSimulationSteps(maxSteps), config(configuration)
{
    dispatcher = std::make_unique<btCollisionDispatcher>(config);
    broadPhase = std::make_unique<btDbvtBroadphase>();
    solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    world = std::make_unique<btDiscreteDynamicsWorld>(dispatcher.get(), broadPhase.get(), solver.get(), config);
    btVector3 grav = btVector3(gravity.x, gravity.y, gravity.z);
    world->setGravity(grav);
}

Physics::~Physics() noexcept
{
    auto& objects = world->getCollisionObjectArray();
    for (Int i = world->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = objects[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState())
            delete body->getMotionState();
        world->removeCollisionObject(obj);
        delete obj;
    }
    world.reset();
    solver.reset();
    broadPhase.reset();
    dispatcher.reset();
}

}   // namespace df