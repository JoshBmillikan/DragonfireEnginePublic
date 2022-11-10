//
// Created by josh on 11/9/22.
//

#pragma once
#include <game.h>
#include <SDL.h>

namespace df {

class GameClient : public Game {
public:
    GameClient(int argc, char** argv);
    ~GameClient() override;
    DF_NO_MOVE_COPY(GameClient)
private:
    class Renderer* renderer;
    SDL_Window* window;
};

}   // namespace df