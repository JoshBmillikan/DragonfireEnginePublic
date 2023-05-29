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

struct TextureIndices {
    UInt32 albedo = 0;
    UInt32 ambient = 0;
    UInt32 diffuse = 0;
    UInt32 specular = 0;
};

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

    [[nodiscard]] vk::Pipeline getPipeline() const { return pipeline; }

    [[nodiscard]] vk::PipelineLayout getPipelineLayout() const { return pipelineLayout; }

    [[nodiscard]] const TextureIndices& getTextureIndices() const { return textureIndices; }

private:
    vk::Pipeline pipeline;
    vk::PipelineLayout pipelineLayout;
    TextureIndices textureIndices;
};

}   // namespace dragonfire