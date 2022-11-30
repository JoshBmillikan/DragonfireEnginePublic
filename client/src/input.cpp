//
// Created by josh on 11/28/22.
//

#include "input.h"
#include "file.h"
#include <nlohmann/json.hpp>

namespace df {
using namespace entt::literals;

static nlohmann::json DEFAULT_BINDINGS = R"([
    {
        "type": "button",
        "name": "forward",
        "key": 119
    },
    {
        "type": "button",
        "name": "backward",
        "key": 115
    },
    {
        "type": "button",
        "name": "left",
        "key": 97
    },
    {
        "type": "button",
        "name": "right",
        "key": 100
    },
    {
        "type": "axis2d",
        "name": "look",
        "axis": "mouse_delta"
    }
])"_json;

void createInputBindings(entt::registry& registry, const char* filepath)
{
    nlohmann::json json;
    try {
        File file(filepath);
        json = nlohmann::json::parse(file.readToString());
        spdlog::info("Loaded input mapping: \"{}\"", filepath);
    }
    catch (...) {
        json = DEFAULT_BINDINGS;
        spdlog::info("Default input mapping loaded");
    }

    for (auto& binding : json) {
        auto name = binding["name"].get<std::string>();
        auto entity = registry.create();
        registry.emplace<std::string>(entity, std::move(name));
        registry.emplace<InputBinding>(entity);
        if (binding.contains("key")) {
            registry.emplace<ButtonState>(entity);
            if (binding["key"].is_number_unsigned()) {
                SDL_KeyCode code = static_cast<SDL_KeyCode>(binding["key"].get<unsigned char>());
                registry.emplace<SDL_KeyCode>(entity, code);
            } else {
                //todo
            }
        }
    }
}

InputComponent getBindingByName(entt::registry& registry, std::string_view name)
{
    for (auto&& [entity, bindingName] : registry.view<std::string, InputBinding>().each()) {
        if (name == bindingName)
            return {entity};
    }
    throw std::out_of_range("No input binding for the giving name");
}
}   // namespace df