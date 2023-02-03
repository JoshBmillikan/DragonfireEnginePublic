//
// Created by josh on 11/17/22.
//

#pragma once
#include <concepts>
#include <mutex>
#include <nlohmann/json_fwd.hpp>
#include <ranges>
namespace df {

class Asset {
public:
    virtual ~Asset() = default;
    [[nodiscard]] const std::string& getName() const noexcept { return name; }

protected:
    std::string name;
};

class AssetRegistry {
public:
    template<typename T>
        requires std::is_base_of_v<Asset, T>
    T* get(const char* name)
    {
        T* ptr = dynamic_cast<T*>(assets.at(name).get());
        if (ptr)
            return ptr;
        throw std::bad_cast();
    }

    void destroy() noexcept;

    static AssetRegistry& getRegistry() { return *instance; }
    AssetRegistry() noexcept;
    template<typename T>
        requires std::is_base_of_v<Asset, T>
    T* operator[](const char* str)
    {
        return get<T>(str);
    }

    template<typename T>
        requires std::is_base_of_v<Asset, T>
    std::vector<T*> allOf()
    {
        std::vector<T*> out;
        for (auto& [name, asset] : assets.values()) {
            T* ptr = dynamic_cast<T*> (asset.get());
            if (ptr)
                out.push_back(ptr);
        }
        return out;
    }

    void insert(std::unique_ptr<Asset>&& asset) { assets[asset->getName()] = std::move(asset); }
    void insert(Asset* asset) { insert(std::unique_ptr<Asset>(asset)); }

    struct Loader {
        virtual std::vector<Asset*> load(const char* filename) = 0;
        virtual ~Loader() = default;
        std::vector<Asset*> operator()(const char* filename) { return load(filename); }

    protected:
        std::shared_ptr<spdlog::logger> logger = spdlog::get("Assets");
        static nlohmann::json readMetadata(const char* filename);
    };
    void loadDir(const char* dirName, Loader& loader);
    void loadFile(const char* filename, Loader& loader);

private:
    std::shared_mutex mutex;
    HashMap<std::string, std::unique_ptr<Asset>> assets;
    std::shared_ptr<spdlog::logger> logger;
    inline static AssetRegistry* instance = nullptr;
};

}   // namespace df