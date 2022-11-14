//
// Created by josh on 11/14/22.
//

#pragma once
#include <array>
#include <nlohmann/json.hpp>

namespace df {

struct GraphicsSettings {
    std::string windowTitle;
    bool vsync = true;
    std::array<UInt, 2> resolution = {800, 600};
    enum class WindowMode {
        borderless,
        fullscreen,
        windowed,
    } windowMode = WindowMode::borderless;
};

class Config {
    static Config instance;
public:
    GraphicsSettings graphics;
    static Config& get() noexcept {return instance;}
    static void loadConfigFile(const char* path);
    void saveConfig(const char* path);
};

NLOHMANN_JSON_SERIALIZE_ENUM(
        GraphicsSettings::WindowMode,
        {
                {GraphicsSettings::WindowMode::borderless, "borderless"},
                {GraphicsSettings::WindowMode::fullscreen, "fullscreen"},
                {GraphicsSettings::WindowMode::windowed, "windowed"},
        }
);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GraphicsSettings, vsync, resolution, windowMode);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config, graphics);
}   // namespace df