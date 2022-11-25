//
// Created by josh on 11/10/22.
//

#include "game.h"
#include "config.h"
#include <chrono>
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
    initLogging(APP_NAME);
    spdlog::info("Loading application \"{}\"", APP_NAME);
    Config::loadConfigFile("config.json");
    spdlog::info("PhysFS initialized");
}

Game::~Game()
{
    assetRegistry.destroy();
    Config::get().saveConfig("config.json");
    PHYSFS_deinit();
    spdlog::info("Goodbye!");
    spdlog::shutdown();
}

static auto START_TIME = std::chrono::steady_clock::now();

void Game::run()
{
    using namespace std::chrono;
    time_point lastTime = steady_clock::now();
    spdlog::info("Startup finished in {} seconds", duration<double>(lastTime - START_TIME).count());
    while (running) {
        time_point now = steady_clock::now();
        auto delta = duration<double>(now - lastTime);
        lastTime = now;
        mainLoop(delta.count());
    }
}

/**
 * Initialize logging
 * @param filename name of the log file to write to
 */
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
    logPath.append(".log");
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
    info("Logging started to {}", logPath);
}
}   // namespace df