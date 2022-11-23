//
// Created by josh on 11/9/22.
//

#include "game_client.h"
#include "../rendering/src/render_asset_loaders.h"

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
    registry.emplace<Model>(entity,"Suzanne", "basic");
    Transform t;
    t.position.y += 2;
    registry.emplace<Transform>(entity, t);
    auto entity2 = registry.create();
    registry.emplace<Model>(entity2,"Suzanne", "basic");
    t.position.y -= 4;
    registry.emplace<Transform>(entity2, t);
}

GameClient::~GameClient()
{
    renderContext->waitForLastFrame();
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
    auto renderObjects = registry.group<Model, Transform>();
    for (auto&& [entity, model, transform] : renderObjects.each()) {
        renderContext->addModel(&model, transform);
    }
    renderContext->drawFrame();
}

void GameClient::loadAssets()
{
    renderContext->loadMaterials("assets/materials");
    renderContext->loadModels("assets/models");
}
}   // namespace df