//
// Created by josh on 11/9/22.
//

#pragma once
#include "input.h"
#include <SDL.h>
#include <game.h>
#include <render_context.h>

namespace df {

class LocalGame : public BaseGame {
public:
    LocalGame(int argc, char** argv);
    ~LocalGame() override;
    DF_NO_MOVE_COPY(LocalGame)
protected:
    void update(double deltaSeconds) override;

private:
    std::unique_ptr<RenderContext> renderContext;
    InputManager input;
    void loadAssets();
    void processSdlEvent(const SDL_Event& event);
    void resetInputs();
};

}   // namespace df