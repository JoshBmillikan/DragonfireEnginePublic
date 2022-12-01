//
// Created by josh on 11/9/22.
//

#pragma once
#include "input.h"
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
    void update(double deltaSeconds) override;

private:
    std::unique_ptr<RenderContext> renderContext;
    void loadAssets();
    void processSdlEvent(const SDL_Event& event);
    void resetInputs();
};

}   // namespace df