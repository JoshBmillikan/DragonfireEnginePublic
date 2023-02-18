//
// Created by josh on 11/28/22.
//

#pragma once
#include <SDL.h>
#include <nlohmann/json_fwd.hpp>

namespace df {
class InputManager {
public:
    InputManager() = default;
    InputManager(const char* filename);
    void processEvent(const SDL_Event& event);
    void saveInputBindingFile(const char* filename);
    void resetInputs();

private:
    HashMap<SDL_KeyCode, std::string> keyBindings;
    glm::ivec2 mouseDelta{}, mousePosition{};

    void loadBindingsFromJson(const nlohmann::json& json);
};
}   // namespace df