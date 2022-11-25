//
// Created by josh on 11/9/22.
//

#include "game_client.h"

namespace df {

GameClient::GameClient(int argc, char** argv) : Game(argc, argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0)
        crash("Failed to initialize SDL: {}", SDL_GetError());
    SDL_version version;
    SDL_GetVersion(&version);
    spdlog::info("SDL version {}.{}.{} loaded", version.major, version.minor, version.patch);
    renderContext = new RenderContext();
    loadAssets();
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

GameClient::~GameClient()
{
    renderContext->stopRendering();
    registry.clear<>();
    assetRegistry.destroy();
    delete renderContext;
    SDL_Quit();
}

void GameClient::mainLoop(double deltaSeconds)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.sym != SDLK_ESCAPE)
                    break;
            case SDL_QUIT:
                spdlog::info("Quit requested");
                stop();
                break;
        }
    }
    update(deltaSeconds);
}

void GameClient::update(double deltaSeconds)
{
    static bool direction = true;
    for (auto&& [entity, transform] : registry.view<Transform>().each()) {
        if (direction) {
            if (3 - transform.position.z < 0.01)
                direction = !direction;
            transform.position = lerp(transform.position, transform.position + glm::vec3(0, 0, 3), (float) deltaSeconds / 5.0f);
        }
        else {
            if (transform.position.z < -1.01)
                direction = !direction;
            transform.position = lerp(transform.position, transform.position + glm::vec3(0, 0, -3), (float) deltaSeconds / 5.0f);
        }
    }

    auto renderObjects = registry.group<Model, Transform>();
    for (auto&& [entity, model, transform] : renderObjects.each()) {
        renderContext->enqueueModel(&model, transform);
    }
    renderContext->drawFrame();
}

void GameClient::loadAssets()
{
    renderContext->loadTextures("assets/textures");
    renderContext->loadMaterials("assets/materials");
    renderContext->loadModels("assets/models");
}
}   // namespace df