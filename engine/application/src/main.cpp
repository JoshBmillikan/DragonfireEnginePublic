//
// Created by josh on 5/17/23.
//

#include "app.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <fmt/format.h>
#include <physfs.h>

using namespace dragonfire;

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) < 0) {
        auto msg = fmt::format("SDL initialization failed: {}", SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_NAME, msg.c_str(), nullptr);
        return -1;
    }
    if (PHYSFS_init(argc > 0 ? *argv : nullptr) == 0) {
        auto error = PHYSFS_getLastErrorCode();
        auto msg = fmt::format("PhysFS init failed: {}", PHYSFS_getErrorByCode(error));
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_NAME, msg.c_str(), nullptr);
        SDL_Quit();
        return error;
    }

    try {
        App::INSTANCE.init();
        App::INSTANCE.run();
        App::INSTANCE.shutdown();
    }
    catch (const std::exception& e) {
        auto msg = fmt::format("An unhandled exception has occurred: {}", e.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_NAME, msg.c_str(), nullptr);
        SDL_Quit();
        return -1;
    }
    catch (...) {
        auto ptr = std::current_exception();
        SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                APP_NAME,
                "The application has encountered an unknown error and must close.",
                nullptr
        );
        SDL_Quit();
        return -1;
    }

    PHYSFS_deinit();
    SDL_Quit();
}