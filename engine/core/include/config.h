//
// Created by josh on 5/18/23.
//

#pragma once
#include <allocators.h>
#include <ankerl/unordered_dense.h>
#include <nlohmann/json_fwd.hpp>
#include <shared_mutex>

namespace dragonfire {

class Config {
public:
    static Config INSTANCE;

    void loadConfigFile(const char* path);
    void loadConfigJson(const nlohmann::json& json, const TempString& root = "");

    template<typename T>
    void setVar(const std::string_view id, T&& val)
    {
        std::unique_lock lock(mutex);
        variables[std::string(id)] = std::forward<T>(val);
    }

    template<typename T>
    T get(const std::string& id)
    {
        std::shared_lock lock(mutex);
        return std::get<T>(variables[id]);
    }

    std::pair<const std::string&, std::shared_lock<std::shared_mutex>> getStrRef(
            const std::string& id
    );

    void saveConfig(const char* path);

private:
    ankerl::unordered_dense::map<std::string, std::variant<Int64, double, bool, std::string>>
            variables;
    std::shared_mutex mutex;
};

}   // namespace dragonfire