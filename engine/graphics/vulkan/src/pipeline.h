//
// Created by josh on 5/21/23.
//

#pragma once
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
            const std::array<vk::DescriptorSetLayout, 4>& setLayouts,
            vk::SampleCountFlagBits multisamplingSamples,
            std::vector<vk::RenderPass>&& renderPasses
    );

    ~PipelineFactory() noexcept { destroy(); };

    void destroy() noexcept;
    void saveCache();
    std::pair<vk::Pipeline, vk::PipelineLayout> createPipeline(const Material::ShaderEffect& effect);

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
    ankerl::unordered_dense::map<USize, vk::Pipeline> builtPipelines;
    ankerl::unordered_dense::map<USize, vk::PipelineLayout> builtLayouts;
    std::array<vk::DescriptorSetLayout, 4> setLayouts;
    vk::SampleCountFlagBits multisamplingSamples = vk::SampleCountFlagBits::e1;
    std::vector<vk::RenderPass> renderPasses;

    std::shared_mutex mutex;

    void loadShaders();
    UInt getShaderStages(std::array<vk::PipelineShaderStageCreateInfo, 5>& infos, const Material::ShaderEffect& effect);
    vk::PipelineLayout getCreateLayout(const Material::ShaderEffect& effect);
    void loadLayoutReflectionData(const std::string& shaderName, PipelineFactory::PipelineLayoutInfo& info);
};
}   // namespace dragonfire