//
// Created by josh on 11/13/22.
//

#pragma once
#include "vulkan_includes.h"
#include <ankerl/unordered_dense.h>
#include <nlohmann/json_fwd.hpp>

namespace df {

class PipelineFactory {
public:
    PipelineFactory() = default;
    PipelineFactory(
            vk::Device device,
            vk::Format imageFormat,
            vk::Format depthFormat,
            spdlog::logger* logger,
            vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1
    );
    ~PipelineFactory() noexcept { destroy(); };
    void destroy() noexcept;
    DF_NO_COPY(PipelineFactory);
    PipelineFactory(PipelineFactory&& other) noexcept;
    PipelineFactory& operator=(PipelineFactory&& other) noexcept;
    vk::Pipeline createPipeline(const nlohmann::json& pipelineDescription, vk::PipelineLayout layout);
    void saveCache();
    static constexpr const char* CACHE_PATH = "cache/shader_pipeline.cache";

private:
    void loadShaders() noexcept;
    UInt getStageCreateInfo(vk::PipelineShaderStageCreateInfo* stages, const nlohmann::json& json);
    HashMap<std::string, vk::ShaderModule> shaders;
    vk::Device device = nullptr;
    vk::PipelineCache cache;
    vk::Format imageFormat{}, depthFormat{};
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
    spdlog::logger* logger = nullptr;
};

}   // namespace df