//
// Created by josh on 5/21/23.
//

#include "pipeline.h"
#include "allocators.h"
#include <file.h>
#include <vulkan/vulkan_hash.hpp>
#include <model.h>

namespace dragonfire {

static std::array<vk::VertexInputBindingDescription, 1> MESH_VERTEX_INPUT_BINDING = {
        vk::VertexInputBindingDescription(0, sizeof(Model::Vertex), vk::VertexInputRate::eVertex),
};

static std::array<vk::VertexInputAttributeDescription, 3> MESH_VERTEX_ATTRIBUTES = {
        vk::VertexInputAttributeDescription(
                0,
                0,
                vk::Format::eR32G32B32Sfloat,
                offsetof(Model::Vertex, position)
        ),
        vk::VertexInputAttributeDescription(
                1,
                0,
                vk::Format::eR32G32B32Sfloat,
                offsetof(Model::Vertex, normal)
        ),
        vk::VertexInputAttributeDescription(
                2,
                0,
                vk::Format::eR32G32Sfloat,
                offsetof(Model::Vertex, uv)
        ),
};

std::pair<vk::Pipeline, vk::PipelineLayout> PipelineFactory::createPipeline(const Material::ShaderEffect& effect)
{
    std::array<vk::PipelineShaderStageCreateInfo, 5> stageInfos;
    UInt stageCount = getShaderStages(stageInfos, effect);
    vk::PipelineLayout layout = getCreateLayout(effect);
    vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;
    dynamicStateCreateInfo.dynamicStateCount = 2;

    vk::PipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.rasterizationSamples = effect.enableMultisampling ? multisamplingSamples
                                                                       : vk::SampleCountFlagBits::e1;
    multisampleState.sampleShadingEnable = effect.enableMultisampling;
    multisampleState.alphaToOneEnable = false;
    multisampleState.alphaToCoverageEnable = false;
    multisampleState.minSampleShading = 0.2f;

    vk::PipelineInputAssemblyStateCreateInfo inputAsm{};
    inputAsm.primitiveRestartEnable = false;
    switch (effect.topology) {
        case Material::ShaderEffect::Topology::triangleList: inputAsm.topology = vk::PrimitiveTopology::eTriangleList; break;
        case Material::ShaderEffect::Topology::triangleFan: inputAsm.topology = vk::PrimitiveTopology::eTriangleFan; break;
        case Material::ShaderEffect::Topology::point: inputAsm.topology = vk::PrimitiveTopology::ePointList; break;
        case Material::ShaderEffect::Topology::list: inputAsm.topology = vk::PrimitiveTopology::eLineList; break;
    }

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.scissorCount = viewportState.viewportCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterState{};
    rasterState.depthBiasEnable = false;
    rasterState.depthClampEnable = false;
    rasterState.rasterizerDiscardEnable = false;
    rasterState.polygonMode = vk::PolygonMode::eFill;
    rasterState.cullMode = vk::CullModeFlagBits::eBack;
    rasterState.frontFace = vk::FrontFace::eCounterClockwise;
    rasterState.lineWidth = 1.0f;

    vk::PipelineDepthStencilStateCreateInfo depthInfo{};
    depthInfo.depthWriteEnable = effect.enableDepth;
    depthInfo.depthTestEnable = effect.enableDepth;
    depthInfo.depthCompareOp = vk::CompareOp::eLessOrEqual;
    depthInfo.depthBoundsTestEnable = false;
    depthInfo.stencilTestEnable = false;
    depthInfo.minDepthBounds = 0.0f;
    depthInfo.maxDepthBounds = 1.0f;

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState{};
    colorBlendAttachmentState.blendEnable = effect.enableColorBlend;
    colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                               | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &colorBlendAttachmentState;
    colorBlend.logicOpEnable = false;

    vk::PipelineVertexInputStateCreateInfo vertexInput{};
    if (effect.topology == Material::ShaderEffect::Topology::triangleList
        || effect.topology == Material::ShaderEffect::Topology::triangleFan) {
        vertexInput.pVertexBindingDescriptions = MESH_VERTEX_INPUT_BINDING.data();
        vertexInput.vertexBindingDescriptionCount = MESH_VERTEX_INPUT_BINDING.size();
        vertexInput.pVertexAttributeDescriptions = MESH_VERTEX_ATTRIBUTES.data();
        vertexInput.vertexAttributeDescriptionCount = MESH_VERTEX_ATTRIBUTES.size();
    }
    else {
        crash("not yet implemented");   // TODO
    }

    vk::GraphicsPipelineCreateInfo createInfo{};
    createInfo.stageCount = stageCount;
    createInfo.pStages = stageInfos.data();
    createInfo.layout = layout;
    createInfo.pDynamicState = &dynamicStateCreateInfo;
    createInfo.pMultisampleState = &multisampleState;
    createInfo.renderPass = renderPasses[effect.renderPassIndex];
    createInfo.subpass = effect.subpass;
    createInfo.pInputAssemblyState = &inputAsm;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterState;
    createInfo.pDepthStencilState = &depthInfo;
    createInfo.pColorBlendState = &colorBlend;
    createInfo.pVertexInputState = &vertexInput;

    auto [result, pipeline] = device.createGraphicsPipeline(cache, createInfo);
    if (result != vk::Result::eSuccess)
        crash("Failed to create graphics pipeline");
    builtPipelines[Material::ShaderEffect::Hash()(effect)] = pipeline;
    return {pipeline, layout};
}

UInt PipelineFactory::getShaderStages(
        std::array<vk::PipelineShaderStageCreateInfo, 5>& infos,
        const Material::ShaderEffect& effect
)
{
    UInt count = 0;
    if (!effect.shaderNames.vertex.empty()) {
        infos[count] = vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eVertex,
                shaders[effect.shaderNames.vertex].first,
                "main"
        );
        count++;
    }
    if (!effect.shaderNames.fragment.empty()) {
        infos[count] = vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eFragment,
                shaders[effect.shaderNames.fragment].first,
                "main"
        );
        count++;
    }
    if (!effect.shaderNames.geometry.empty()) {
        infos[count] = vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eGeometry,
                shaders[effect.shaderNames.geometry].first,
                "main"
        );
        count++;
    }
    if (!effect.shaderNames.tessEval.empty()) {
        infos[count] = vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eTessellationEvaluation,
                shaders[effect.shaderNames.tessEval].first,
                "main"
        );
        count++;
    }
    if (!effect.shaderNames.tessCtrl.empty()) {
        infos[count] = vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eTessellationControl,
                shaders[effect.shaderNames.tessCtrl].first,
                "main"
        );
        count++;
    }
    return count;
}

vk::PipelineLayout PipelineFactory::getCreateLayout(const Material::ShaderEffect& effect)

{
    PipelineLayoutInfo layoutInfo;
    loadLayoutReflectionData(effect.shaderNames.vertex, layoutInfo);
    loadLayoutReflectionData(effect.shaderNames.fragment, layoutInfo);
    loadLayoutReflectionData(effect.shaderNames.geometry, layoutInfo);
    loadLayoutReflectionData(effect.shaderNames.tessEval, layoutInfo);
    loadLayoutReflectionData(effect.shaderNames.tessCtrl, layoutInfo);

    USize hash = PipelineLayoutInfo::Hash()(layoutInfo);
    {
        std::shared_lock lock(mutex);
        if (builtLayouts.contains(hash))
            return builtLayouts[hash];
    }

    std::array<vk::DescriptorSetLayout, 4> usedSetLayouts;
    UInt setLayoutCount = 0;
    for (UInt i = 0; i < setLayouts.size(); i++) {
        if (layoutInfo.sets[i]) {
            usedSetLayouts[setLayoutCount] = setLayouts[i];
            setLayoutCount++;
        }
    }
    vk::PipelineLayoutCreateInfo createInfo{};
    createInfo.pPushConstantRanges = layoutInfo.pushConstants.data();
    createInfo.pushConstantRangeCount = layoutInfo.pushConstants.size();
    createInfo.pSetLayouts = usedSetLayouts.data();
    createInfo.setLayoutCount = setLayoutCount;

    std::unique_lock lock(mutex);
    if (builtLayouts.contains(hash))
        return builtLayouts[hash];
    vk::PipelineLayout layout = device.createPipelineLayout(createInfo);
    builtLayouts[hash] = layout;
    return layout;
}

void PipelineFactory::loadLayoutReflectionData(const std::string& shaderName, PipelineFactory::PipelineLayoutInfo& info)
{
    if (shaderName.empty())
        return;
    if (!shaders.contains(shaderName))
        throw FormattedError("Requested shader module \"{}\" not available", shaderName);
    const auto& shader = shaders[shaderName].second;

    for (UInt i = 0; i < shader.push_constant_block_count; i++) {
        const auto& pushInfo = shader.push_constant_blocks[i];
        info.pushConstants.emplace_back(
                static_cast<vk::ShaderStageFlagBits>(shader.shader_stage),
                pushInfo.offset,
                pushInfo.size
        );
    }

    for (UInt i = 0; i < shader.descriptor_set_count; i++) {
        const auto& set = shader.descriptor_sets[i];
        UInt setIndex = set.set;
        if (setIndex > 4)
            spdlog::error("Descriptor set count greater than 4 in shader \"{}\" not supported", shaderName);
        setIndex = std::min(setIndex, 4u);
        info.sets[setIndex] = true;
    }
}

PipelineFactory::PipelineFactory(
        vk::Device device,
        const std::array<vk::DescriptorSetLayout, 4>& setLayouts,
        vk::SampleCountFlagBits multisamplingSamples,
        std::vector<vk::RenderPass>&& renderPasses
)
    : device(device),
      setLayouts(setLayouts),
      multisamplingSamples(multisamplingSamples),
      renderPasses(std::move(renderPasses))
{
    logger = spdlog::get("Rendering");
    try {
        vk::PipelineCacheCreateInfo createInfo{};
        File file("cache/pipeline.cache");
        auto data = file.readData();
        file.close();
        createInfo.initialDataSize = data.size();
        createInfo.pInitialData = data.data();
        cache = device.createPipelineCache(createInfo);
    }
    catch (const std::exception& e) {
        logger->error("Failed to load pipeline cache(this is normal on the first run)");
        cache = device.createPipelineCache(vk::PipelineCacheCreateInfo());
    }
    loadShaders();
}

void PipelineFactory::saveCache()
{
    PHYSFS_mkdir("cache");
    auto data = device.getPipelineCacheData(cache);
    File file("cache/pipeline.cache", File::Mode::write);
    file.writeData(data);
    logger->info("Saved pipeline cache to disk");
}

void PipelineFactory::loadShaders()
{
#ifdef SHADER_OUTPUT_PATH
    if (PHYSFS_mount(SHADER_OUTPUT_PATH, "assets/shaders/", 0) == 0)
        spdlog::error("Failed to mount shader dir");
#endif
    char** files = PHYSFS_enumerateFiles("assets/shaders");
    if (files == nullptr)
        throw std::runtime_error("Failed to load shader directory");
    for (char** i = files; *i != nullptr; i++) {
        try {
            char* ext = strrchr(*i, '.');
            if (!(ext && strcmp(ext, ".spv") == 0))
                continue;
            TempString path = "assets/shaders/";
            path += *i;
            File file(path.c_str());
            auto spv = file.readData();
            file.close();
            std::string name = *i;
            name.erase(name.rfind(".spv"));
            auto& pair = shaders[std::move(name)];
            if (spvReflectCreateShaderModule(spv.size(), spv.data(), &pair.second) != SPV_REFLECT_RESULT_SUCCESS)
                throw std::runtime_error("Shader reflection failed");
            vk::ShaderModuleCreateInfo createInfo{};
            createInfo.codeSize = spvReflectGetCodeSize(&pair.second);
            createInfo.pCode = spvReflectGetCode(&pair.second);
            pair.first = device.createShaderModule(createInfo);
            logger->info("Loaded shader module \"{}\"", *i);
        }
        catch (const std::exception& e) {
            logger->error("Failed to load shader module \"{}\", error: {}", *i, e.what());
        }
    }
    PHYSFS_freeList(files);
}

void PipelineFactory::destroy() noexcept
{
    if (device) {
        for (auto& [name, modules] : shaders) {
            device.destroy(modules.first);
            spvReflectDestroyShaderModule(&modules.second);
        }
        try {
            saveCache();
        }
        catch (const std::exception& e) {
            logger->error("Failed to save pipeline cache: {}", e.what());
        }
        device.destroy(cache);
        device = nullptr;
    }
}

bool PipelineFactory::PipelineLayoutInfo::operator==(const PipelineFactory::PipelineLayoutInfo& other) const
{
    if (sets.size() != other.sets.size() || pushConstants.size() != other.pushConstants.size())
        return false;
    if (other.sets != sets)
        return false;
    for (UInt i = 0; i < pushConstants.size(); i++) {
        if (pushConstants[i] != other.pushConstants[i])
            return false;
    }
    return flags == other.flags;
}

USize PipelineFactory::PipelineLayoutInfo::Hash::operator()(const PipelineFactory::PipelineLayoutInfo& layoutInfo) const
{
    USize result = std::hash<USize>()(layoutInfo.sets.size()) ^ std::hash<USize>()(layoutInfo.pushConstants.size());
    for (auto set : layoutInfo.sets)
        result ^= std::hash<bool>()(set);
    for (auto& range : layoutInfo.pushConstants)
        result ^= std::hash<vk::PushConstantRange>()(range);
    result ^= std::hash<vk::Flags<vk::PipelineLayoutCreateFlagBits>>()(layoutInfo.flags);
    return result;
}
}   // namespace dragonfire