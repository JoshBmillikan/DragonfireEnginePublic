//
// Created by josh on 11/9/22.
//
#include "local_game.h"
#include "ui/ui.h"
#include <SDL2/SDL_main.h>

// declared extern c for compatability with SDL_main
extern "C" int main(int argc, char** argv)
{
    auto ui = df::ui::initCEF(argc, argv);
    df::LocalGame game(argc, argv, std::move(ui));
    game.run();
    CefShutdown();
}