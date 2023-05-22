//
// Created by josh on 5/21/23.
//

#include "vk_material.h"
#include "pipeline.h"
#include "vk_renderer.h"
#include <file.h>
#include <nlohmann/json.hpp>
#include <utility.h>
#include <vulkan/vulkan_hash.hpp>

namespace dragonfire {
Material* VkMaterial::Library::getMaterial(const std::string& name)
{
    if (materials.contains(name))
        return &materials[name];
    return nullptr;
}

void VkMaterial::Library::loadMaterialFiles(const char* dir, Renderer* renderer)
{
    VkRenderer* render = static_cast<VkRenderer*>(renderer);
    device = render->getDevice();
    auto logger = spdlog::get("Rendering");
    std::unique_ptr<char*, decltype([](char** ptr) { PHYSFS_freeList(ptr); })> files(PHYSFS_enumerateFiles(dir));
    UInt fileCount = 0;
    for (char** i = files.get(); i != nullptr; i++)
        fileCount++;
    if (fileCount == 0)
        throw FormattedError("No material files found in dir \"{}\"", dir);
    PipelineFactory pipelineFactory(
            device,
            render->getDescriptorSetLayouts(),
            render->getSampleCount(),
            render->getRenderPasses()
    );

    struct ThreadData {
        ankerl::unordered_dense::set<vk::Pipeline> pipelines;
        ankerl::unordered_dense::set<vk::PipelineLayout> layouts;
        ankerl::unordered_dense::map<std::string, VkMaterial> loadedMaterials;
    };

    BS::multi_future<ThreadData> future =
            GLOBAL_THREAD_POOL.parallelize_loop(fileCount, [&](const UInt start, const UInt end) {
                ThreadData data;
                for (UInt i = start; i < end; i++) {
                    try {
                        nlohmann::json json = loadJson(files.get()[i]);
                        std::string name = json["name"].get<std::string>();
                        ShaderEffect effect(json["effect"]);
                        auto [pl, layout] = pipelineFactory.createPipeline(effect);
                        data.pipelines.insert(pl);
                        data.layouts.insert(layout);
                        logger->info("Loaded material \"{}\"", name);
                        data.loadedMaterials[std::move(name)] = VkMaterial(pl, layout);
                        // TODO other material info
                    }
                    catch (const std::exception& e) {
                        logger->error("Failed to load material file \"{}\", error: {}", files.get()[i], e.what());
                    }
                }
                return data;
            });

    std::vector<ThreadData> data = future.get();
    for (ThreadData& d : data) {
        createdPipelines.insert(d.pipelines.begin(), d.pipelines.end());
        createdLayouts.insert(d.layouts.begin(), d.layouts.end());
        materials.insert(d.loadedMaterials.begin(), d.loadedMaterials.end());
    }
}

void VkMaterial::Library::destroy()
{
    if (device) {
        for (vk::Pipeline pl : createdPipelines)
            device.destroy(pl);
        for (vk::PipelineLayout l : createdLayouts)
            device.destroy(l);
        materials.clear();
        device = nullptr;
    }
}
}   // namespace dragonfire