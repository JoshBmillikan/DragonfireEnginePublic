//
// Created by josh on 11/17/22.
//
#include "asset.h"
#include "util.h"
#include <physfs.h>
namespace df {

void AssetRegistry::loadDir(const char* dirName)
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
            loadFile(path);
        }
        catch (const std::runtime_error& e) {
            logger->error("Failed to load asset file: {}, error: {}", *ptr, e.what());
        }
    }
    PHYSFS_freeList(files);
}

void AssetRegistry::loadFile(const char* filename)
{
    const char* extension = getFileExtension(filename);
    std::shared_lock lock(mutex);
    auto asset = std::unique_ptr<Asset>(loaders.at(extension)->load(filename));
    lock.unlock();
    std::unique_lock unique(lock);
    assets[asset->getName()] = std::move(asset);
}

void AssetRegistry::destroy() noexcept
{
    assets.clear();
    loaders.clear();
}

AssetRegistry::AssetRegistry() noexcept
{
    instance = this;
    logger = spdlog::default_logger()->clone("Assets");
    spdlog::register_logger(logger);
}
}   // namespace df
