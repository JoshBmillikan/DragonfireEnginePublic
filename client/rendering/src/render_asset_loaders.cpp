//
// Created by josh on 11/19/22.
//
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#define TINYOBJLOADER_IMPLEMENTATION
#include "render_asset_loaders.h"
#include "renderer.h"
#include "stb_image.h"
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
    vk::PipelineLayoutCreateInfo createInfo;
    createInfo.pSetLayouts = &setLayout;
    createInfo.setLayoutCount = 1;

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

Material* MaterialLoader::createMaterial(nlohmann::json& json)
{
    Material* material = nullptr;
    vk::Pipeline pipeline = nullptr;
    vk::PipelineLayout layout = createPipelineLayout(json);
    try {
        pipeline = pipelineFactory->createPipeline(json["shaders"], layout);
        material = new Material();
        material->pipeline = pipeline;
        material->layout = layout;
        material->device = device;
        material->name = json["name"].get<std::string>();
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

    File file(filename);
    auto data = file.readBytes();
    file.close();
    int width, height, pixelSize;
    stbi_uc* pixels = stbi_load_from_memory(data.data(), (int) data.size(), &width, &height, &pixelSize, STBI_rgb_alpha);

    //todo
    return {};
}

}   // namespace df
