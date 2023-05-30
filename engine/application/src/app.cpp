//
// Created by josh on 5/17/23.
//

#include "app.h"
#include <SDL.h>
#include <allocators.h>
#include <config.h>
#include <file.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <math.h>
#include <model.h>
#include <physfs.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <transform.h>

namespace dragonfire {

App App::INSTANCE;

void App::update(double deltaTime)
{
    for (auto&& [entity, transform] : world.getRegistry().view<Transform>().each()) {
        transform.rotation = glm::rotate(transform.rotation, float(deltaTime * 1.0), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    renderer->startImGuiFrame();
    ImGui::Begin("Test");
    ImGui::Text("Hello world");
    ImGui::Text("Frame time: %.1fms (%.1f FPS)", deltaTime * 1000, ImGui::GetIO().Framerate);
    ImGui::End();
    ImGui::Render();
    renderer->render(world, camera);
}

void App::processEvents(double deltaTime)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        switch (event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.scancode != SDL_SCANCODE_ESCAPE)
                    break;
            case SDL_QUIT: stop(); break;
        }
    }
}

void App::run()
{
    running = true;
    UInt64 time = SDL_GetTicks64();
    spdlog::info("Initialization finished in {:.3}", double(time) / 1000.0);
    while (running) {
        frame_allocator::nextFrame();
        UInt64 now = SDL_GetTicks64();
        double dt = double(now - time) / 1000.0;
        processEvents(dt);
        update(dt);
        time = now;
    }
}

void App::shutdown()
{
    renderer->shutdown();
    Config::INSTANCE.saveConfig("settings.json");
    spdlog::info("Goodbye!");
    spdlog::shutdown();
}

static void initLogging()
{
    using namespace spdlog;
    const char* str = std::getenv("LOG_LEVEL");
    level::level_enum level = level::info;
    if (str) {
        if (strcasecmp(str, "debug") == 0)
            level = level::debug;
        else if (strcasecmp(str, "warn") == 0)
            level = level::warn;
        else if (strcasecmp(str, "trace") == 0)
            level = level::trace;
        else if (strcasecmp(str, "err") == 0)
            level = level::err;
    }
    const char* writeDir = PHYSFS_getWriteDir();
    assert("Please initialize PhysFs write path before logging" && writeDir);
    std::string logPath = writeDir;
    logPath.append("log.txt");
    init_thread_pool(8192, 1);

    auto stdoutSink = std::make_shared<sinks::stdout_color_sink_mt>();
    auto fileSink = std::make_shared<sinks::basic_file_sink_mt>(logPath);
    auto logger = std::make_shared<async_logger>(
            "global",
            sinks_init_list{stdoutSink, fileSink},
            thread_pool(),
            async_overflow_policy::block
    );
    register_logger(logger);
    set_default_logger(logger);
    set_level(level);
    set_pattern("[%T] [%^%=8!l%$] [%=12!n] %v %@");
    info("Logging started to {}", logPath);
}

static void spawnBunnies(entt::registry& registry, Renderer* renderer)
{
    auto model = Model::loadGltfModel("assets/models/bunny.glb", renderer);
    const UInt32 bunnyCount = 24;
    const UInt32 rowCount = 6;
    for (UInt i = 0; i < rowCount; i++) {
        for (UInt32 j = 0; j < bunnyCount; j++) {
            auto entity = registry.create();
            registry.emplace<Model>(entity, model);
            auto& t = registry.emplace<Transform>(entity);
            t.position.y -= float(j + 1);
            t.position.x += 1.0f + (float(j) * 0.005f) + (2.0f * float(i));
            t.position.z -= 0.5f;
            t.scale *= 2;
        }
    }
}

void App::init()
{
    int err = PHYSFS_setSaneConfig("org", APP_ID, "pak", false, false);
    if (err != 0)
        err = PHYSFS_mount(ASSET_PATH, "assets", false);
    if (err == 0)
        crash("PhysFS init failed: {}", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    initLogging();
    spdlog::info("Asset path: {}", ASSET_PATH);
    spdlog::info("Write dir: {}", PHYSFS_getWriteDir());
    spdlog::info("Current platform: {}", SDL_GetPlatform());
    SDL_version version;
    SDL_GetVersion(&version);
    spdlog::info("SDL version {}.{}.{} loaded", version.major, version.minor, version.patch);
    Config::INSTANCE.loadConfigFile("assets/default_settings.json");
    try {
        Config::INSTANCE.loadConfigFile("settings.json");
    }
    catch (const PhysFSError&) {
        Config::INSTANCE.saveConfig("settings.json");
    }
    renderer = getRenderer();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    renderer->init();
    float width = float(Config::INSTANCE.get<Int64>("graphics.window.resolution.0"));
    float height = float(Config::INSTANCE.get<Int64>("graphics.window.resolution.1"));
    camera = Camera(60.0f, width, height);
    auto model = Model::loadGltfModel("assets/models/dragon.glb", renderer);

    auto& registry = world.getRegistry();
    auto entity = registry.create();
    registry.emplace<Model>(entity, std::move(model));
    auto& t = registry.emplace<Transform>(entity);
    t.position.y -= 20;
    t.position.x -= 2;
    t.scale *= 0.05f;

    spawnBunnies(registry, renderer);
    camera.lookAt(t.position);
    t.position.x -= 2;
}

}   // namespace dragonfire