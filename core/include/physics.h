//
// Created by josh on 11/30/22.
//

#pragma once
#include <bullet/btBulletCollisionCommon.h>
#include <bullet/btBulletDynamicsCommon.h>
#include <entt/entt.hpp>

namespace df {

class Physics {
public:
    explicit Physics(
            glm::vec3 gravity = glm::vec3(0.0f, 0.0f, -9.8f),
            btCollisionConfiguration* configuration = &defaultConfig,
            Int maxSteps = 10
    );
    ~Physics() noexcept;
    void stepSimulation(double deltaTime) { world->stepSimulation((float) (deltaTime / 60.0), maxSimulationSteps); };
    DF_NO_MOVE_COPY(Physics);
    const Int maxSimulationSteps;

private:
    static btDefaultCollisionConfiguration defaultConfig;
    static btDbvtBroadphase defaultBroadPhase;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btBroadphaseInterface> broadPhase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
    std::unique_ptr<btDiscreteDynamicsWorld> world;
    btCollisionConfiguration* config;
};

}   // namespace df