//
// Created by josh on 11/17/22.
//

#pragma once
#include <concepts>
#include <mutex>
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
    AssetRegistry() noexcept { instance = this; }
    template<typename T>
        requires std::is_base_of_v<Asset, T>
    T* operator[](const char* str)
    {
        return get<T>(str);
    }
    void loadDir(const char* dirName);
    void loadFile(const char* filename);

    struct Loader {
        virtual Asset* load(const char* filename) = 0;
        virtual ~Loader() = default;
        Asset* operator()(const char* filename) { return load(filename); }
    };

    void addLoader(std::unique_ptr<Loader>&& loader, const char* fileExtension)
    {
        loaders.emplace(fileExtension, std::move(loader));
    }

private:
    std::shared_mutex mutex;
    HashMap<std::string, std::unique_ptr<Asset>> assets;
    HashMap<const char*, std::unique_ptr<Loader>> loaders;
    inline static AssetRegistry* instance = nullptr;
};

}   // namespace df