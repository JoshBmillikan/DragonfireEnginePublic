//
// Created by josh on 11/10/22.
//

#include "game.h"
#include "config.h"
#include <ctime>
#include <physfs.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#ifndef ASSET_PATH
    #define ASSET_PATH "../assets"
#endif

namespace df {
static void initLogging(const char* filename);
Game::Game(int argc, char** argv)
{
    int err = PHYSFS_init(argc > 0 ? *argv : nullptr);
    if (err != 0)
        err = PHYSFS_setSaneConfig("org", APP_NAME, "zip", false, false);
    if (err != 0)
        err = PHYSFS_mount(ASSET_PATH, "assets", false);
    if (err == 0)
        crash("PhysFS initialization failed: {}", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    initLogging(argc > 0 ? *argv : "game.log");
    spdlog::info("Loading application \"{}\"", APP_NAME);
    Config::loadConfigFile("config.json");
    spdlog::info("PhysFS initialized");
}

Game::~Game()
{
    Config::get().saveConfig("config.json");
    PHYSFS_deinit();
    spdlog::info("Goodbye!");
    spdlog::shutdown();
}

void Game::run()
{
    clock_t lastTime = std::clock();
    spdlog::info("Startup finished in {} seconds", (float) lastTime / CLOCKS_PER_SEC);
    while (running) {
        clock_t now = std::clock();
        double delta = (double) (now - lastTime) / CLOCKS_PER_SEC;
        lastTime = now;
        mainLoop(delta);
    }
}

void initLogging(const char* filename)
{
    using namespace spdlog;
    const char* str = std::getenv("DF_LOG_LEVEL");
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
    logPath.append("/log/");
    logPath.append(filename);
    init_thread_pool(8192, 1);

    auto stdoutSink = std::make_shared<sinks::stdout_color_sink_mt>();
    auto fileSink = std::make_shared<sinks::basic_file_sink_mt>(logPath);
    auto logger = std::make_shared<async_logger>(
            "Global",
            sinks_init_list{stdoutSink, fileSink},
            thread_pool(),
            async_overflow_policy::block
    );
    register_logger(logger);
    set_default_logger(logger);
    set_level(level);
    set_pattern("[%T] [thread:%5t] [%^%=8!l%$] [%=12!n] %v %@");
    info("Logging started");
}
}   // namespace df