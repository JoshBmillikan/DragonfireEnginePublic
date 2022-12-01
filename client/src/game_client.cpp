//
// Created by josh on 11/9/22.
//

#include "game_client.h"

namespace df {
using namespace entt::literals;

GameClient::GameClient(int argc, char** argv) : Game(argc, argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0)
        crash("Failed to initialize SDL: {}", SDL_GetError());
    SDL_version version;
    SDL_GetVersion(&version);
    spdlog::info("SDL version {}.{}.{} loaded", version.major, version.minor, version.patch);
    renderContext = std::make_unique<RenderContext>();
    loadAssets();
}

GameClient::~GameClient()
{
    renderContext->stopRendering();
    world.reset();
    assetRegistry.destroy();
    renderContext.reset();
    SDL_Quit();
}

void GameClient::update(double deltaSeconds)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        processSdlEvent(event);
    if (world) {
        world->update(deltaSeconds);

        auto renderObjects = world->getRegistry().group<Model, const Transform>();
        for (auto&& [entity, model, transform] : renderObjects.each())
            renderContext->enqueueModel(&model, transform);
    }
    renderContext->drawFrame();
    resetInputs();
}

void GameClient::processSdlEvent(const SDL_Event& event)
{
    switch (event.type) {
        case SDL_QUIT:
            spdlog::info("Quit requested");
            stop();
            break;
        case SDL_KEYUP:
        case SDL_KEYDOWN:
            if (event.key.repeat == 0) {

            }
            break;
        case SDL_MOUSEMOTION:
            break;
    }
}

void GameClient::resetInputs()
{
}

void GameClient::loadAssets()
{
    renderContext->loadTextures("assets/textures");
    renderContext->loadMaterials("assets/materials");
    renderContext->loadModels("assets/models");
}
}   // namespace df