//
// Created by josh on 5/17/23.
//

#include "app.h"
#include <SDL2/SDL.h>
#include <allocators.h>
#include <config.h>
#include <file.h>
#include <physfs.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace dragonfire {

App App::INSTANCE;

void App::update(double deltaTime)
{
}

void App::processEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
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
        processEvents();
        UInt64 now = SDL_GetTicks64();
        double dt = double(now - time) / 1000.0;
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
    renderer->init();
}

}   // namespace dragonfire