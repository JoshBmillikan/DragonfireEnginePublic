//
// Created by josh on 11/28/22.
//

#include "input.h"
#include "file.h"
#include <nlohmann/json.hpp>

namespace df {

void InputManager::processEvent(const SDL_Event& event)
{
    switch (event.type) {
    }
}

static void setKeyBinding(HashMap<SDL_KeyCode, std::string>& map, const std::string& name, const nlohmann::json& json)
{
    SDL_KeyCode code = SDLK_UNKNOWN;
    if (json.is_number_integer())
        code = json.get<SDL_KeyCode>();
    else if (json.is_string())
        code = static_cast<SDL_KeyCode>(SDL_GetKeyFromName(json.get<std::string>().c_str()));

    if (code != SDLK_UNKNOWN) {
        map.emplace(code, name);
        spdlog::info(R"(Loaded key binding "{}" for key "{}")", name, SDL_GetKeyName(code));
    }
    else
        spdlog::error("Invalid key binding: \"{}\"", to_string(json));
}

void InputManager::loadBindingsFromJson(const nlohmann::json& json)
{
    for (auto& binding : json.items()) {
        if (binding.value().contains("key"))
            setKeyBinding(keyBindings, binding.key(), binding.value()["key"]);
    }
}

void InputManager::saveInputBindingFile(const char* filename)
{
    nlohmann::json json;
    for (auto& [key, name] : keyBindings) {
        nlohmann::json binding;
        binding.emplace("key", SDL_GetKeyName(key));
        json[name] = binding;
    }

    File file(filename, File::Mode::write);
    file.writeString(json.dump(2));
    spdlog::info("Input bindings saved");
}

static nlohmann::json DEFAULT_BINDINGS = R"({
    "forward": {
        "key": 119
    },
    "backward": {
        "key": 115
    },
    "left": {
        "key": 97
    },
    "right": {
        "key": 100
    },
    "look": {
        "axis2d": "mouse_delta"
    }
})"_json;

InputManager::InputManager(const char* filename)
{
    try {
        File file(filename);
        nlohmann::json json = nlohmann::json::parse(file.readToString());
        file.close();
        loadBindingsFromJson(json);
    }
    catch (...) {
        spdlog::warn("Failed to load input bindings file");
        loadBindingsFromJson(DEFAULT_BINDINGS);
    }
}
}   // namespace df