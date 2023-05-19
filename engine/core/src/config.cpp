//
// Created by josh on 5/18/23.
//

#include "config.h"
#include <nlohmann/json.hpp>
#include <utility.h>

namespace dragonfire {
Config Config::INSTANCE;

void Config::loadConfigFile(const char* path)
{
    try {
        nlohmann::json json = loadJson(path);
        loadConfigJson(json);
        spdlog::info("Loaded config file \"{}\"", path);
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to load config file \"{}\", error: {}", path, e.what());
        throw;
    }
}

void Config::loadConfigJson(const nlohmann::json& json, const TempString& root)
{
    if (json.is_object() || json.is_array()) {
        for (const auto& [key, value] : json.items()) {
            TempString str = root;
            if (!str.empty())
                str += ".";
            str += key;
            loadConfigJson(value, str);
        }
    }
    else if (json.is_boolean()) {
        bool val = json.get<bool>();
        setVar(root, val);
        spdlog::trace("Loaded config var \"{}\" of type boolean", root);
    }
    else if (json.is_number_integer()) {
        int64_t val = json.get<int64_t>();
        setVar(root, val);
        spdlog::trace("Loaded config var \"{}\" of type integer", root);
    }
    else if (json.is_number_float()) {
        double val = json.get<double>();
        setVar(root, val);
        spdlog::trace("Loaded config var \"{}\" of type float", root);
    }
    else if (json.is_string()) {
        TempString val = json.get<TempString>();
        setVar(root, std::string(val));
        spdlog::trace("Loaded config var \"{}\" of type string", root);
    }
    else
        throw std::runtime_error("Unsupported JSON entry type");
}

std::pair<const std::string&, std::shared_lock<std::shared_mutex>> Config::getStrRef(
        const std::string& id
)
{
    std::shared_lock lock(mutex);
    const std::string& str = std::get<std::string>(variables[id]);
    return {str, std::move(lock)};
}

void Config::saveConfig(const char* path)
{
    // TODO
}

}   // namespace dragonfire