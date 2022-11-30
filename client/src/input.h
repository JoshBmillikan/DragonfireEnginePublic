//
// Created by josh on 11/28/22.
//

#pragma once
#include "entt/entity/registry.hpp"
#include <SDL.h>

namespace df {

struct ButtonState {
    bool pressed = false;
    bool released = false;
    bool changedThisFrame = false;

    UShort modifiers = 0;
};

struct AxisState {
    float value = 0;
    float multiplier = 1;
};

struct Axis2DState {
    float x = 0, y = 0;
    operator glm::vec2() const noexcept { return glm::vec2(x, y); }
    float multiplier = 1;
};

struct InputBinding {};
struct InputComponent {
    entt::entity entity;
    ButtonState& getButton(entt::registry& registry) const noexcept { return registry.get<ButtonState>(entity); }
    AxisState& getAxis(entt::registry& registry) const noexcept { return registry.get<AxisState>(entity); }
    Axis2DState& getAxis2D(entt::registry& registry) const noexcept { return registry.get<Axis2DState>(entity); }
};

void createInputBindings(entt::registry& registry, const char* filepath = "input_mapping.json");
InputComponent getBindingByName(entt::registry& registry, std::string_view name);
}   // namespace df