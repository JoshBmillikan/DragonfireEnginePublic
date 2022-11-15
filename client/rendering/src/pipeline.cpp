//
// Created by josh on 11/13/22.
//

#include "pipeline.h"
#include "file.h"
#include "mesh.h"
#include <nlohmann/json.hpp>

namespace df {

static vk::PipelineCache loadCache(vk::Device device);

PipelineFactory::PipelineFactory(
        vk::Device device,
        vk::Format imageFormat,
        vk::Format depthFormat,
        spdlog::logger* logger,
        vk::SampleCountFlagBits msaaSamples
)
    : device(device), imageFormat(imageFormat), depthFormat(depthFormat), msaaSamples(msaaSamples), logger(logger)
{
    try {
        cache = loadCache(device);
    }
    catch (const std::exception& e) {
        logger->warn("Failed to load pipeline cache(this is normal on the first run): {}", e.what());
        vk::PipelineCacheCreateInfo createInfo;
        cache = device.createPipelineCache(createInfo);
    }
    loadShaders();
}

vk::Pipeline PipelineFactory::createPipeline(const nlohmann::json& pipelineDescription, vk::PipelineLayout layout)
{
    vk::PipelineShaderStageCreateInfo stageInfos[5];
    const UInt stageCount = getStageCreateInfo(stageInfos, pipelineDescription);

    vk::PipelineVertexInputStateCreateInfo vertexInput;
    vertexInput.pVertexAttributeDescriptions = Mesh::vertexAttributeDescriptions.data();
    vertexInput.vertexAttributeDescriptionCount = Mesh::vertexAttributeDescriptions.size();
    vertexInput.pVertexBindingDescriptions = Mesh::vertexInputDescriptions.data();
    vertexInput.vertexBindingDescriptionCount = Mesh::vertexInputDescriptions.size();

    vk::PipelineDepthStencilStateCreateInfo depth;
    depth.depthBoundsTestEnable = true;
    depth.depthWriteEnable = true;
    depth.depthCompareOp = vk::CompareOp::eLess;
    depth.depthBoundsTestEnable = false;
    depth.stencilTestEnable = false;
    depth.minDepthBounds = 0;
    depth.maxDepthBounds = 1;

    vk::DynamicState dynamicStates[2] = {vk::DynamicState::eScissor, vk::DynamicState::eViewport};
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
    dynamicStateInfo.dynamicStateCount = 2;
    dynamicStateInfo.pDynamicStates = dynamicStates;

    vk::PipelineViewportStateCreateInfo viewport;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    vk::PipelineInputAssemblyStateCreateInfo inputAsm;
    inputAsm.topology = vk::PrimitiveTopology::eTriangleList;
    inputAsm.primitiveRestartEnable = false;

    vk::PipelineRasterizationStateCreateInfo rasterState;
    rasterState.depthClampEnable = false;
    rasterState.rasterizerDiscardEnable = false;
    rasterState.polygonMode = vk::PolygonMode::eFill;
    rasterState.cullMode = vk::CullModeFlagBits::eBack;
    rasterState.frontFace = vk::FrontFace::eCounterClockwise;
    rasterState.depthBiasEnable = false;
    rasterState.lineWidth = 1;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.rasterizationSamples = msaaSamples;
    multisampling.sampleShadingEnable = true;
    multisampling.minSampleShading = 0.2;
    multisampling.alphaToOneEnable = false;
    multisampling.alphaToCoverageEnable = false;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.blendEnable = false;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                          | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlend;
    colorBlend.logicOpEnable = false;
    colorBlend.pAttachments = &colorBlendAttachment;
    colorBlend.attachmentCount = 1;

    vk::PipelineRenderingCreateInfo renderState;
    renderState.pColorAttachmentFormats = &imageFormat;
    renderState.colorAttachmentCount = 1;

    vk::GraphicsPipelineCreateInfo createInfo;
    createInfo.pNext = &renderState;
    createInfo.pDynamicState = &dynamicStateInfo;
    createInfo.pVertexInputState = &vertexInput;
    createInfo.pViewportState = &viewport;
    createInfo.layout = layout;
    createInfo.pMultisampleState = &multisampling;
    createInfo.stageCount = stageCount;
    createInfo.pStages = stageInfos;
    createInfo.pInputAssemblyState = &inputAsm;
    createInfo.pRasterizationState = &rasterState;
    createInfo.pDepthStencilState = &depth;
    createInfo.pColorBlendState = &colorBlend;
    createInfo.renderPass = nullptr;

    auto [result, pipeline] = device.createGraphicsPipeline(cache, createInfo);
    vk::resultCheck(result, "Failed to create graphics pipeline");
    return pipeline;
}

UInt PipelineFactory::getStageCreateInfo(vk::PipelineShaderStageCreateInfo* stages, const nlohmann::json& json)
{
    UInt count = 2;
    stages[0].stage = vk::ShaderStageFlagBits::eVertex;
    stages[0].module = shaders[json["vertex"].get<std::string>()];
    stages[0].pName = "main";
    stages[1].stage = vk::ShaderStageFlagBits::eFragment;
    stages[1].module = shaders[json["fragment"].get<std::string>()];
    stages[1].pName = "main";

    if (json.contains("geometry")) {
        stages[count].stage = vk::ShaderStageFlagBits::eGeometry;
        stages[count].module = shaders[json["geometry"].get<std::string>()];
        stages[count].pName = "main";
        count++;
    }

    if (json.contains("tessCtrl")) {
        stages[count].stage = vk::ShaderStageFlagBits::eTessellationControl;
        stages[count].module = shaders[json["tessCtrl"].get<std::string>()];
        stages[count].pName = "main";
        count++;
    }

    if (json.contains("tessEval")) {
        stages[count].stage = vk::ShaderStageFlagBits::eTessellationEvaluation;
        stages[count].module = shaders[json["tessEval"].get<std::string>()];
        stages[count].pName = "main";
        count++;
    }

    return count;
}

void PipelineFactory::loadShaders() noexcept
{
    char** files = PHYSFS_enumerateFiles("assets/shaders");
    if (files == nullptr)
        crash("Failed to enumerate files in shader directory: {}", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    std::vector<UInt> data;
    data.reserve(1024);
    char name[40];
    for (char* filename = *files; filename != nullptr; filename = *++files) {
        try {
            File file(filename);
            auto length = file.length();
            if (length < 4)
                throw std::runtime_error("SPIR-V file was empty");
            if (data.size() < length)
                data.resize((length / 4) + 1);
            data[0] = 0;
            file.readBytes(data.data(), length);
            file.close();
            if (data[0] != 0x07230203)
                throw std::runtime_error("SPIR-V magic number was incorrect, file is possibly corrupt");
            vk::ShaderModuleCreateInfo createInfo;
            createInfo.codeSize = length;
            createInfo.pCode = data.data();
            vk::ShaderModule module = device.createShaderModule(createInfo);
            Long len = 40;
            char* dot = strrchr(filename, '.');
            if (dot) {
                auto distance = std::distance(filename, dot);
                len = std::clamp(distance, 1l, len);
            }
            strncpy(name, filename, len);
            name[len - 1] = '\0';
            shaders.emplace(name, module);
        }
        catch (const std::exception& e) {
            logger->error("Failed to create shader module \"{}\", error: {}", filename, e.what());
        }
    }
    PHYSFS_freeList(files);
}

void PipelineFactory::saveCache()
{
    auto data = device.getPipelineCacheData(cache);
    PHYSFS_mkdir("cache");
    File file(CACHE_PATH, File::write);
    file.writeBytes(data.data(), data.size());
    logger->info("Saved pipeline cache to disk");
}

vk::PipelineCache loadCache(vk::Device device) {
   File file(PipelineFactory::CACHE_PATH);
   auto data = file.readBytes();
   vk::PipelineCacheCreateInfo createInfo;
   createInfo.initialDataSize = data.size();
   createInfo.pInitialData = data.data();
   return device.createPipelineCache(createInfo);
}

void PipelineFactory::destroy() noexcept
{
    if (device) {
        for (auto& shader : shaders)
            device.destroy(shader.second);
        try {
            saveCache();
        }
        catch (const std::exception& e) {
            logger->error("Failed to save pipeline cache to disk: {}", e.what());
        }
        device.destroy(cache);
        device = nullptr;
    }
}

PipelineFactory::PipelineFactory(PipelineFactory&& other) noexcept
{
    if (this != &other) {
        shaders = std::move(other.shaders);
        device = other.device;
        cache = other.cache;
        imageFormat = other.imageFormat;
        depthFormat = other.depthFormat;
        msaaSamples = other.msaaSamples;
        logger = other.logger;
        other.device = nullptr;
    }
}

PipelineFactory& PipelineFactory::operator=(PipelineFactory&& other) noexcept
{
    if (this != &other) {
        shaders = std::move(other.shaders);
        device = other.device;
        cache = other.cache;
        imageFormat = other.imageFormat;
        depthFormat = other.depthFormat;
        msaaSamples = other.msaaSamples;
        logger = other.logger;
        other.device = nullptr;
    }
    return *this;
}
}   // namespace df