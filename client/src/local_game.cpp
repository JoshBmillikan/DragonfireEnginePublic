//
// Created by josh on 11/9/22.
//

#include "local_game.h"
#include "world/local_world.h"

namespace df {
using namespace entt::literals;

LocalGame::LocalGame(int argc, char** argv) : BaseGame(argc, argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0)
        crash("Failed to initialize SDL: {}", SDL_GetError());
    SDL_version version;
    SDL_GetVersion(&version);
    spdlog::info("SDL version {}.{}.{} loaded", version.major, version.minor, version.patch);
    renderContext = std::make_unique<RenderContext>();
    loadAssets();
    world = std::make_unique<LocalWorld>("Test world",random());
}

LocalGame::~LocalGame()
{
    renderContext->stopRendering();
    world.reset();
    assetRegistry.destroy();
    renderContext.reset();
    SDL_Quit();
}

void LocalGame::update(double deltaSeconds)
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

void LocalGame::processSdlEvent(const SDL_Event& event)
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
        case SDL_MOUSEWHEEL:
            break;
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
            break;
    }
}

void LocalGame::resetInputs()
{
}

void LocalGame::loadAssets()
{
    renderContext->loadTextures("assets/textures");
    renderContext->loadMaterials("assets/materials");
    renderContext->loadModels("assets/models");
}
}   // namespace df