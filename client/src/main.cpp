//
// Created by josh on 11/9/22.
//
#include <SDL2/SDL_main.h>
#include "game_client.h"

extern "C" int main(int argc, char** argv) {
    df::GameClient game(argc, argv);
    game.run();
}