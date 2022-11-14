//
// Created by josh on 11/14/22.
//

#include "config.h"
#include "file.h"

namespace df {
Config Config::instance;
void Config::loadConfigFile(const char* path)
{
    try {
        File file(path);
        nlohmann::json json = nlohmann::json::parse(file.readToString(), nullptr, true, true);
        spdlog::trace("Loaded config file: {}", json);
        instance = json.get<Config>();
    }
    catch (const std::exception& e) {
        instance = Config();
        spdlog::warn("Failed to load config file \'{}\', error: {}", path, e.what());
    }
}

void Config::saveConfig(const char* path)
{
    nlohmann::json json = nlohmann::json(*this);
    File file(path, File::Mode::write);
    file.writeString(json.dump(2));
    spdlog::info("Wrote config file to disk");
}
}   // namespace df