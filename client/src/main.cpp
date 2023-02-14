//
// Created by josh on 11/9/22.
//
#include "local_game.h"
#include <SDL_main.h>

// declared extern c for compatability with SDL_main
extern "C" int main(int argc, char** argv)
{
    df::LocalGame game(argc, argv);
    game.run();
}