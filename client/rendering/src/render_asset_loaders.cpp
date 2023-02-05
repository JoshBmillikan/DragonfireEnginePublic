//
// Created by josh on 11/19/22.
//
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#define TINYOBJLOADER_IMPLEMENTATION
#include "render_asset_loaders.h"
#include "renderer.h"
#include "stb_image.h"
#include "util.h"
#include <asset.h>
#include <file.h>
#include <mutex>
#include <nlohmann/json.hpp>

namespace df {
std::vector<Asset*> ObjLoader::load(const char* filename)
{
    File file(filename);
    std::string str = file.readToString();
    file.close();
    bool ok = reader.ParseFromString(str, "");
    if (!ok) {
        if (!reader.Error().empty())
            throw std::runtime_error(reader.Error());
    }
    if (!reader.Warning().empty())
        logger->warn("OBJ loader warning: {}", reader.Warning());

    auto& attributes = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    std::vector<Asset*> out;
    out.reserve(shapes.size());
    for (auto& shape : shapes) {
        auto mesh = createMesh(shape, attributes);
        mesh->setName(shape.name);
        out.push_back(mesh);
    }
    return out;
}

Mesh* ObjLoader::createMesh(const tinyobj::shape_t& shape, const tinyobj::attrib_t& attributes)
{
    std::unique_lock lock(mutex);
    vertices.clear();
    indices.clear();
    uniqueVertices.clear();
    vertices.reserve(shape.mesh.num_face_vertices.size());
    indices.reserve(shape.mesh.indices.size());
    uniqueVertices.reserve(shape.mesh.indices.size());
    for (const auto& index : shape.mesh.indices) {
        Vertex vertex{
                .position =
                        {
                                attributes.vertices[3 * index.vertex_index + 0],
                                attributes.vertices[3 * index.vertex_index + 1],
                                attributes.vertices[3 * index.vertex_index + 2],
                        },
                .normal =
                        {
                                attributes.normals[3 * index.normal_index + 0],
                                attributes.normals[3 * index.normal_index + 1],
                                attributes.normals[3 * index.normal_index + 2],
                        },
                .uv =
                        {
                                attributes.texcoords[2 * index.texcoord_index + 0],
                                1.0f - attributes.texcoords[2 * index.texcoord_index + 1],
                        },
        };
        if (!uniqueVertices.contains(vertex)) {
            uniqueVertices[vertex] = vertices.size();
            vertices.emplace_back(vertex);
        }
        indices.emplace_back(uniqueVertices[vertex]);
    }
    logger->info("Loaded model \"{}\" with {} vertices", shape.name, vertices.size());
    return factory.create(vertices, indices);
}

vk::PipelineLayout MaterialLoader::createPipelineLayout(nlohmann::json& json)
{
    vk::PushConstantRange pushConstantRange{};
    pushConstantRange.offset = 0;
    pushConstantRange.size = Material::TEXTURE_COUNT * sizeof(UInt);
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
    vk::PipelineLayoutCreateInfo createInfo;
    createInfo.pSetLayouts = &setLayout;
    createInfo.setLayoutCount = 1;
    createInfo.pPushConstantRanges = &pushConstantRange;
    createInfo.pushConstantRangeCount = 1;

    return device.createPipelineLayout(createInfo);
}

std::vector<Asset*> MaterialLoader::load(const char* filename)
{
    File file(filename);
    nlohmann::json json = nlohmann::json::parse(file.readToString());
    file.close();
    std::vector<Asset*> out;
    if (json.is_array()) {
        out.reserve(json.size());
        for (auto& material : json)
            out.push_back(createMaterial(material));
    }
    else
        out.push_back(createMaterial(json));

    return out;
}

static Texture* getTexture(const std::string& name, const nlohmann::json& json, spdlog::logger& logger)
{
    try {
        auto texture = json.at("textures").at(name).get<std::string>();
        try {
            return AssetRegistry::getRegistry().get<Texture>(texture);
        }
        catch (...) {
            logger.warn("Missing {} texture (id {}) for material \"{}\"", name, texture, json.at("name").get<std::string>());
            try {
                return AssetRegistry::getRegistry().get<Texture>("error_texture");
            }
            catch (...) {
                logger.error("Could not get error texture!");
                return nullptr;
            }
        }
    }
    catch (...) {
        return nullptr;
    }
}

Material* MaterialLoader::createMaterial(nlohmann::json& json)
{
    Material* material = nullptr;
    vk::Pipeline pipeline = nullptr;
    vk::PipelineLayout layout = createPipelineLayout(json);
    try {
        pipeline = pipelineFactory->createPipeline(json.at("shaders"), layout);
        material = new Material();
        material->pipeline = pipeline;
        material->layout = layout;
        material->device = device;
        material->name = json.at("name").get<std::string>();
        material->albedo = getTexture("albedo", json, *logger);
        material->roughness = getTexture("roughness", json, *logger);
        material->emissive = getTexture("emissive", json, *logger);
        material->normal = getTexture("normal", json, *logger);
        logger->info("Loaded material \"{}\"", material->name);
    }
    catch (std::exception& e) {
        device.destroy(layout);
        if (pipeline)
            device.destroy(pipeline);
        delete material;
        throw;
    }
    return material;
}

MaterialLoader::MaterialLoader(PipelineFactory* pipelineFactory, vk::Device device, vk::DescriptorSetLayout setLayout)
    : pipelineFactory(pipelineFactory), device(device), setLayout(setLayout)
{
}

std::vector<Asset*> PngLoader::load(const char* filename)
{
    int width, height, pixelSize;
    stbi_uc* pixels;
    {
        File file(filename);
        auto data = file.readBytes();
        pixels = stbi_load_from_memory(data.data(), (int) data.size(), &width, &height, &pixelSize, STBI_rgb_alpha);
    }
    if (pixels == nullptr)
        throw std::runtime_error(
                fmt::format("STB image failed to load image \"{}\", reason: {}", filename, stbi_failure_reason())
        );

    vk::Extent2D extent(width, height);
    try {
        Texture* texture = factory.create(extent, pixels, width * height * pixelSize);
        stbi_image_free(pixels);
        std::string name = nameFromPath(filename);
        texture->setName(name);
        logger->info("Loaded texture \"{}\" at index {}", name, texture->getIndex());
        return {texture};
    }
    catch (...) {
        stbi_image_free(pixels);
        throw;
    }
}

}   // namespace df
