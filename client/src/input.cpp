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
        "name": "forward",
        "key": 119
    },
    {
        "name": "backward",
        "key": 115
    },
    {
        "name": "left",
        "key": 97
    },
    {
        "name": "right",
        "key": 100
    },
    {
        "name": "look",
        "axis2d": "mouse_delta"
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
        registry.emplace<InputBinding>(entity, name.c_str());
        registry.emplace<std::string>(entity, std::move(name));
        if (binding.contains("key")) {
            registry.emplace<ButtonState>(entity);
            if (binding["key"].is_number_unsigned()) {
                SDL_KeyCode code = static_cast<SDL_KeyCode>(binding["key"].get<unsigned char>());
                registry.emplace<SDL_KeyCode>(entity, code);
            }
            else if (binding["key"].is_string()) {
                auto str = binding["key"].get<std::string>();
                unsigned char c = str[0];
                registry.emplace<SDL_KeyCode>(entity, static_cast<SDL_KeyCode>(c));
            }
            else
                spdlog::error("Invalid keybinding for {}", binding["key"]);
        }

        if (binding.contains("axis")) {
            registry.emplace<AxisState>(entity);
            // todo
        }

        if (binding.contains("axis2d")) {
            registry.emplace<Axis2DState>(entity);
            // todo
        }
    }
}

InputComponent getBindingByName(entt::registry& registry, std::string_view name)
{
    for (auto&& [entity, bindingName, binding] : registry.view<std::string, InputBinding>().each()) {
        if (name == bindingName)
            return {entity};
    }
    throw std::out_of_range("No input binding for the giving name");
}
}   // namespace df