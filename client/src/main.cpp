//
// Created by josh on 11/9/22.
//
#include "game_client.h"
#include <SDL2/SDL_main.h>

extern "C" int main(int argc, char** argv)
{
    df::GameClient game(argc, argv);
    game.run();
}