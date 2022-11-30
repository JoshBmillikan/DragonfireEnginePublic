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
    renderContext = new RenderContext();
    loadAssets();
    createInputBindings(registry);
    auto entity = registry.create();
    registry.emplace<Model>(entity, "Suzanne", "basic");
    Transform t;
    t.position.y += 2;
    registry.emplace<Transform>(entity, t);
    auto entity2 = registry.create();
    registry.emplace<Model>(entity2, "Suzanne", "basic");
    t.position.y -= 4;
    registry.emplace<Transform>(entity2, t);
    registry.emplace<InputComponent>(entity, getBindingByName(registry, "forward"));
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
    while (SDL_PollEvent(&event))
        processSdlEvents(event);
    update(deltaSeconds);
    resetInputs();
}

void GameClient::processSdlEvents(const SDL_Event& event)
{
    switch (event.type) {
        case SDL_QUIT:
            spdlog::info("Quit requested");
            stop();
            break;
        case SDL_KEYUP:
        case SDL_KEYDOWN:
            if (event.key.repeat == 0) {
                for (auto&& [entity, key, button] : registry.view<const SDL_KeyCode, ButtonState>().each()) {
                    if (event.key.keysym.sym == key) {
                        button.pressed = event.key.state == SDL_PRESSED;
                        button.released = event.key.state == SDL_RELEASED;
                        button.modifiers = event.key.keysym.mod;
                    }
                }
            }
            break;
        case SDL_MOUSEMOTION:
            for (auto&& [entity, axis] : registry.view<Axis2DState, entt::tag<"mouse_receiver"_hs>>().each()) {
                axis.x = (float) event.motion.xrel * axis.multiplier;
                axis.y = (float) event.motion.yrel * axis.multiplier;
            }
            break;
    }
}

void GameClient::update(double deltaSeconds)
{
    for (auto&& [entity, transform, input] : registry.view<Transform, InputComponent>().each()) {
        auto& button = input.getButton(registry);
        if (button.pressed)
            transform.position.z = lerp(transform.position.z, transform.position.z + 1.0f, (float)deltaSeconds);
    }

    auto renderObjects = registry.group<Model, Transform>();
    for (auto&& [entity, model, transform] : renderObjects.each())
        renderContext->enqueueModel(&model, transform);
    renderContext->drawFrame();
}

void GameClient::resetInputs()
{
    for (auto&& [entity, button] : registry.view<ButtonState>().each()) {
        button.changedThisFrame = false;
        button.released = false;
        button.modifiers = 0;
    }
    for (auto&& [entity, axis] : registry.view<AxisState>().each())
        axis.value = 0;
    for (auto&& [entity, axis] : registry.view<Axis2DState>().each())
        axis.x = axis.y = 0;
}

void GameClient::loadAssets()
{
    renderContext->loadTextures("assets/textures");
    renderContext->loadMaterials("assets/materials");
    renderContext->loadModels("assets/models");
}
}   // namespace df