//
// Created by josh on 11/17/22.
//
#include "asset.h"
#include "file.h"
#include "util.h"
#include <nlohmann/json.hpp>
namespace df {

void AssetRegistry::loadDir(const char* dirName, Loader& loader)
{
    char** files = PHYSFS_enumerateFiles(dirName);
    char path[MAX_FILEPATH_LENGTH];
    const auto dirNameLen = strlen(dirName);
    if (dirNameLen > MAX_FILEPATH_LENGTH - 1 || dirNameLen < 1)
        crash("Invalid directory name: {}", dirName);
    for (char** ptr = files; *ptr != nullptr; ptr++) {
        strncpy(path, dirName, MAX_FILEPATH_LENGTH);
        if (path[dirNameLen - 1] != '/')
            strcat(path, "/");
        strncat(path, *ptr, MAX_FILEPATH_LENGTH - dirNameLen);
        try {
            loadFile(path, loader);
            logger->info("Loaded asset file \"{}\"", *ptr);
        }
        catch (const std::runtime_error& e) {
            logger->error("Failed to load asset file: {}, error: {}", *ptr, e.what());
        }
    }
    PHYSFS_freeList(files);
}

void AssetRegistry::loadFile(const char* filename, Loader& loader)
{
    const char* extension = getFileExtension(filename);
    if (strcmp(extension, "meta") != 0) {
        std::shared_lock lock(mutex);
        auto loadedAssets = loader.load(filename);
        lock.unlock();
        std::unique_lock unique(lock);
        for (Asset* asset : loadedAssets)
            insert(asset);
    }
}

void AssetRegistry::destroy() noexcept
{
    assets.clear();
}

AssetRegistry::AssetRegistry() noexcept
{
    instance = this;
    if (spdlog::get("Assets") == nullptr) {
        logger = spdlog::default_logger()->clone("Assets");
        spdlog::register_logger(logger);
    }
}

nlohmann::json AssetRegistry::Loader::readMetadata(const char* filename)
{
    char path[MAX_FILEPATH_LENGTH + 6];
    strncpy(path, filename, MAX_FILEPATH_LENGTH);
    strcat(path, ".meta");
    File file(path);
    return file.readToString();
}
}   // namespace df
