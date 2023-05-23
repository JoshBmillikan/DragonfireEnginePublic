//
// Created by josh on 5/21/23.
//

#pragma once
#include "pipeline.h"
#include "renderer.h"
#include <ankerl/unordered_dense.h>
#include <material.h>
#include <vulkan/vulkan_hash.hpp>

namespace dragonfire {

class VkMaterial : public Material {
public:
    VkMaterial(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout)
        : pipeline(pipeline), pipelineLayout(pipelineLayout)
    {
    }

    VkMaterial() = default;

    class VkLibrary : public Material::Library {
    public:
        VkLibrary() = default;
        Material* getMaterial(const std::string& name) override;
        void loadMaterialFiles(const char* dir, Renderer* renderer, PipelineFactory& pipelineFactory);

        void destroy();

    private:
        vk::Device device;
        ankerl::unordered_dense::map<std::string, VkMaterial> materials;
        ankerl::unordered_dense::set<vk::Pipeline> createdPipelines;
        ankerl::unordered_dense::set<vk::PipelineLayout> createdLayouts;
    };

private:
    vk::Pipeline pipeline;
    vk::PipelineLayout pipelineLayout;
};

}   // namespace dragonfire