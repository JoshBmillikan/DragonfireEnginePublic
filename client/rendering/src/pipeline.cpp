//
// Created by josh on 11/13/22.
//

#include "pipeline.h"
#include "file.h"
#include "mesh.h"
#include <nlohmann/json.hpp>

#ifndef SHADER_DIR
    #define SHADER_DIR "./shaders"
#endif

namespace df {

void PipelineInfo::build(vk::Pipeline* pipelines, UInt count)
{
    vk::PipelineShaderStageCreateInfo stageInfos[5];
    assert("Pipeline description must not be null" && pipelineDescription);
    UInt stageCount = parent.getStageCreateInfo(stageInfos, *pipelineDescription);

    vk::DynamicState dynamicStates[2] = {vk::DynamicState::eScissor, vk::DynamicState::eViewport};
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
    dynamicStateInfo.dynamicStateCount = 2;
    dynamicStateInfo.pDynamicStates = dynamicStates;

    vk::PipelineColorBlendStateCreateInfo colorBlend;
    colorBlend.logicOpEnable = false;
    colorBlend.pAttachments = &colorBlendAttachment;
    colorBlend.attachmentCount = 1;

    assert(count > 0);
    auto createInfos = (vk::GraphicsPipelineCreateInfo*) alloca(count * sizeof(vk::GraphicsPipelineCreateInfo));
    for (UInt i = 0; i < count; i++) {
        createInfos[i] = vk::GraphicsPipelineCreateInfo();
        createInfos[i].pDynamicState = &dynamicStateInfo;
        createInfos[i].pVertexInputState = &vertexInput;
        createInfos[i].pViewportState = &viewport;
        createInfos[i].layout = layout;
        createInfos[i].pMultisampleState = &multisampling;
        createInfos[i].stageCount = stageCount;
        createInfos[i].pStages = stageInfos;
        createInfos[i].pInputAssemblyState = &inputAsm;
        createInfos[i].pRasterizationState = &rasterState;
        createInfos[i].pDepthStencilState = &depth;
        createInfos[i].pColorBlendState = &colorBlend;
        createInfos[i].renderPass = renderPass;
        createInfos[i].subpass = 0;
    }

    vk::resultCheck(
            parent.device.createGraphicsPipelines(parent.cache, count, createInfos, nullptr, pipelines),
            "Failed to create graphics pipeline"
    );
}

PipelineInfo::PipelineInfo(PipelineFactory* parent) : parent(*parent)
{
    renderPass = parent->mainPass;
    vertexInput.pVertexAttributeDescriptions = Mesh::vertexAttributeDescriptions.data();
    vertexInput.vertexAttributeDescriptionCount = Mesh::vertexAttributeDescriptions.size();
    vertexInput.pVertexBindingDescriptions = Mesh::vertexInputDescriptions.data();
    vertexInput.vertexBindingDescriptionCount = Mesh::vertexInputDescriptions.size();

    depth.depthTestEnable = true;
    depth.depthWriteEnable = true;
    depth.depthCompareOp = vk::CompareOp::eLessOrEqual;
    depth.depthBoundsTestEnable = false;
    depth.stencilTestEnable = false;
    depth.minDepthBounds = 0;
    depth.maxDepthBounds = 1;

    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    inputAsm.topology = vk::PrimitiveTopology::eTriangleList;
    inputAsm.primitiveRestartEnable = false;

    rasterState.depthClampEnable = false;
    rasterState.rasterizerDiscardEnable = false;
    rasterState.polygonMode = vk::PolygonMode::eFill;
    rasterState.cullMode = vk::CullModeFlagBits::eBack;
    rasterState.frontFace = vk::FrontFace::eCounterClockwise;
    rasterState.depthBiasEnable = false;
    rasterState.lineWidth = 1;

    multisampling.rasterizationSamples = parent->msaaSamples;
    multisampling.sampleShadingEnable = true;
    multisampling.minSampleShading = 0.2;
    multisampling.alphaToOneEnable = false;
    multisampling.alphaToCoverageEnable = false;

    colorBlendAttachment.blendEnable = false;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                          | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
}

static vk::PipelineCache loadCache(vk::Device device);

PipelineFactory::PipelineFactory(
        vk::Device device,
        vk::RenderPass renderPass,
        spdlog::logger* logger,
        vk::SampleCountFlagBits msaaSamples
)
    : device(device), mainPass(renderPass), msaaSamples(msaaSamples), logger(logger)
{
    if (PHYSFS_mount(SHADER_DIR, "assets/shaders", false) == 0)
        throw PhysFsException();
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
    char** files;
    char** list;
    list = files = PHYSFS_enumerateFiles("assets/shaders");
    if (files == nullptr)
        crash("Failed to enumerate files in shader directory: {}", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    std::vector<UInt> data;
    data.reserve(1024);
    char path[MAX_FILEPATH_LENGTH];
    const auto dirNameLen = strlen("assets/shaders");
    for (char* filename = *files; filename != nullptr; filename = *++files) {
        strncpy(path, "assets/shaders", MAX_FILEPATH_LENGTH);
        if (path[dirNameLen - 1] != '/')
            strcat(path, "/");
        strncat(path, filename, MAX_FILEPATH_LENGTH - dirNameLen);
        try {
            File file(path);
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

            std::string name = filename;
            auto pos = name.rfind('.');
            if (pos != std::string::npos)
                name.erase(pos);
            if (shaders.contains(name))
                logger->warn("Shader module \"{}\" already loaded, it will be overwritten!", name);
            shaders.emplace(name, module);
            logger->info("loaded shader module \"{}\"", name);
        }
        catch (const std::exception& e) {
            logger->error("Failed to create shader module \"{}\", error: {}", filename, e.what());
        }
    }
    PHYSFS_freeList(list);
}

void PipelineFactory::saveCache()
{
    auto data = device.getPipelineCacheData(cache);
    PHYSFS_mkdir("cache");
    File file(CACHE_PATH, File::write);
    file.writeBytes(data.data(), data.size());
    logger->info("Saved pipeline cache to disk");
}

vk::PipelineCache loadCache(vk::Device device)
{
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
        msaaSamples = other.msaaSamples;
        logger = other.logger;
        mainPass = other.mainPass;
        other.device = nullptr;
    }
}

PipelineFactory& PipelineFactory::operator=(PipelineFactory&& other) noexcept
{
    if (this != &other) {
        shaders = std::move(other.shaders);
        device = other.device;
        cache = other.cache;
        msaaSamples = other.msaaSamples;
        logger = other.logger;
        mainPass = other.mainPass;
        other.device = nullptr;
    }
    return *this;
}
}   // namespace df