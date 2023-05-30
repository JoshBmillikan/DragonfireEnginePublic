//
// Created by josh on 5/21/23.
//

#pragma once
#include "descriptor_set.h"
#include "renderer.h"
#include <allocators.h>
#include <ankerl/unordered_dense.h>
#include <material.h>
#include <shared_mutex>
#include <spirv_reflect.h>

namespace dragonfire {

class PipelineFactory {
public:
    PipelineFactory(
            vk::Device device,
            vk::SampleCountFlagBits multisamplingSamples,
            DescriptorLayoutManager* layoutManager,
            vk::DeviceSize maxDrawCount,
            std::vector<vk::RenderPass>&& renderPasses
    );

    ~PipelineFactory() noexcept { destroy(); };

    void destroy() noexcept;
    void saveCache();
    std::pair<vk::Pipeline, vk::PipelineLayout> createPipeline(const Material::ShaderEffect& effect);
    std::pair<vk::Pipeline, vk::PipelineLayout> createComputePipeline(
            const std::string& shaderName,
            vk::PipelineCreateFlagBits flags = {}
    );

    PipelineFactory(PipelineFactory&) = delete;
    PipelineFactory(PipelineFactory&&) = delete;
    PipelineFactory& operator=(PipelineFactory&) = delete;
    PipelineFactory& operator=(PipelineFactory&&) = delete;

private:
    struct PipelineLayoutInfo {
        std::array<bool, 4> sets{};
        vk::Flags<vk::PipelineLayoutCreateFlagBits> flags;
        std::vector<vk::PushConstantRange, FrameAllocator<vk::PushConstantRange>> pushConstants;
        bool operator==(const PipelineLayoutInfo& other) const;

        struct Hash {
            USize operator()(const PipelineLayoutInfo& layoutInfo) const;
        };
    };

    std::shared_ptr<spdlog::logger> logger;
    vk::Device device;
    vk::PipelineCache cache;
    ankerl::unordered_dense::map<std::string, std::pair<vk::ShaderModule, SpvReflectShaderModule>> shaders;
    ankerl::unordered_dense::map<USize, vk::PipelineLayout> builtLayouts;
    DescriptorLayoutManager* layoutManager = nullptr;
    vk::SampleCountFlagBits multisamplingSamples = vk::SampleCountFlagBits::e1;
    std::vector<vk::RenderPass> renderPasses;
    vk::DeviceSize maxDrawCount;

    std::shared_mutex mutex;

    void loadShaders();
    UInt getShaderStages(std::array<vk::PipelineShaderStageCreateInfo, 5>& infos, const Material::ShaderEffect& effect);
    vk::PipelineLayout getCreateLayout(const Material::ShaderEffect& effect);
    vk::PipelineLayout getCreateLayout(
            const PipelineLayoutInfo& layoutInfo,
            std::array<DescriptorLayoutManager::LayoutInfo, 4>& setLayoutInfos
    );
    void loadLayoutReflectionData(
            const std::string& shaderName,
            PipelineFactory::PipelineLayoutInfo& info,
            std::array<DescriptorLayoutManager::LayoutInfo, 4>& layoutInfos
    );
};

class Pipeline {
public:
    Pipeline(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout)
        : pipeline(pipeline), pipelineLayout(pipelineLayout)
    {
    }

    Pipeline() = default;

    class PipelineLibrary {
    public:
        PipelineLibrary() = default;
        Pipeline getPipeline(const std::string& name);
        void loadMaterialFiles(const char* dir, Renderer* renderer, PipelineFactory& pipelineFactory);

        void destroy();

    private:
        vk::Device device;
        ankerl::unordered_dense::map<std::string, Pipeline> pipelines;
        ankerl::unordered_dense::set<vk::Pipeline> createdPipelines;
        ankerl::unordered_dense::set<vk::PipelineLayout> createdLayouts;
    };

    [[nodiscard]] vk::Pipeline getPipeline() const { return pipeline; }

    [[nodiscard]] vk::PipelineLayout getPipelineLayout() const { return pipelineLayout; }

    vk::Pipeline pipeline;
    vk::PipelineLayout pipelineLayout;
};
}   // namespace dragonfire