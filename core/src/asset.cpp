//
// Created by josh on 11/17/22.
//
#include "asset.h"
#include "util.h"
namespace df {

void AssetRegistry::loadDir(const char* dirName)
{
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
}
