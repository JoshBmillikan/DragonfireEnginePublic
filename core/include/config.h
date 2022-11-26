//
// Created by josh on 11/14/22.
//

#pragma once
#include <array>
#include <nlohmann/json.hpp>

namespace df {

struct GraphicsSettings {
    std::string windowTitle;
    float fov = 60;
    bool vsync = true;
    std::array<UInt, 2> resolution = {800, 600};
    enum class WindowMode {
        borderless,
        fullscreen,
        windowed,
    } windowMode = WindowMode::borderless;
    enum class AntiAliasingMode {
        none,
        msaa2,
        msaa4,
        msaa8,
        msaa16,
        msaa32,
        msaa64,
    } antiAliasing = AntiAliasingMode::none;
};

class Config {
    static Config instance;

public:
    GraphicsSettings graphics;
    static Config& get() noexcept { return instance; }
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

NLOHMANN_JSON_SERIALIZE_ENUM(
        GraphicsSettings::AntiAliasingMode,
        {
                {GraphicsSettings::AntiAliasingMode::none, "none"},
                {GraphicsSettings::AntiAliasingMode::msaa2, "msaa2x"},
                {GraphicsSettings::AntiAliasingMode::msaa4, "msaa4x"},
                {GraphicsSettings::AntiAliasingMode::msaa8, "msaa8x"},
                {GraphicsSettings::AntiAliasingMode::msaa16, "msaa16x"},
                {GraphicsSettings::AntiAliasingMode::msaa32, "msaa32x"},
                {GraphicsSettings::AntiAliasingMode::msaa64, "msaa64x"},
        }
);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GraphicsSettings, vsync, resolution, windowMode, fov, antiAliasing);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config, graphics);
}   // namespace df