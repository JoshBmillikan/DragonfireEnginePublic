//
// Created by josh on 11/13/22.
//

#pragma once
#include "vulkan_includes.h"
#include <ankerl/unordered_dense.h>
#include <nlohmann/json_fwd.hpp>

namespace df {
class PipelineInfo {
    class PipelineFactory& parent;

public:
    PipelineInfo(PipelineFactory* parent);
    vk::PipelineLayout layout = nullptr;
    vk::RenderPass renderPass = nullptr;
    vk::PipelineVertexInputStateCreateInfo vertexInput;
    vk::PipelineDepthStencilStateCreateInfo depth;
    vk::PipelineViewportStateCreateInfo viewport;
    vk::PipelineMultisampleStateCreateInfo multisampling;
    vk::PipelineInputAssemblyStateCreateInfo inputAsm;
    vk::PipelineRasterizationStateCreateInfo rasterState;
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    const nlohmann::json* pipelineDescription = nullptr;
    bool enableColorBlend = false;

    vk::Pipeline build()
    {
        vk::Pipeline pipeline;
        build(&pipeline, 1);
        return pipeline;
    }
    void build(vk::Pipeline* pipelines, UInt count);

    vk::Pipeline operator()() { return build(); }
    void operator()(vk::Pipeline* pipelines, UInt count) { return build(pipelines, count); }
};

class PipelineFactory {
    friend class PipelineInfo;

public:
    PipelineFactory() = default;
    PipelineFactory(
            vk::Device device,
            vk::RenderPass renderPass,
            spdlog::logger* logger,
            vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1
    );
    ~PipelineFactory() noexcept { destroy(); };
    void destroy() noexcept;
    DF_NO_COPY(PipelineFactory);
    PipelineFactory(PipelineFactory&& other) noexcept;
    PipelineFactory& operator=(PipelineFactory&& other) noexcept;
    vk::Pipeline createPipeline(const nlohmann::json& pipelineDescription, vk::PipelineLayout layout, vk::RenderPass renderPass)
    {
        auto builder = getBuilder();
        builder.pipelineDescription = &pipelineDescription;
        builder.layout = layout;
        builder.renderPass = renderPass;
        return builder();
    }
    vk::Pipeline createPipeline(const nlohmann::json& pipelineDescription, vk::PipelineLayout layout)
    {
        return createPipeline(pipelineDescription, layout, mainPass);
    };
    void saveCache();
    PipelineInfo getBuilder() { return PipelineInfo(this); }
    static constexpr const char* CACHE_PATH = "cache/shader_pipeline.cache";

private:
    void loadShaders() noexcept;
    UInt getStageCreateInfo(vk::PipelineShaderStageCreateInfo* stages, const nlohmann::json& json);
    HashMap<std::string, vk::ShaderModule> shaders;
    vk::Device device = nullptr;
    vk::PipelineCache cache;
    vk::RenderPass mainPass;
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
    spdlog::logger* logger = nullptr;
};

}   // namespace df