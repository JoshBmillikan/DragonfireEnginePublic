//
// Created by josh on 11/9/22.
//

#pragma once
#include <SDL.h>
#include <game.h>
#include <render_context.h>

namespace df {

class GameClient : public Game {
public:
    GameClient(int argc, char** argv);
    ~GameClient() override;
    DF_NO_MOVE_COPY(GameClient)
protected:
    void mainLoop(double deltaSeconds) override;

private:
    RenderContext* renderContext;
    void loadAssets();
};

}   // namespace df