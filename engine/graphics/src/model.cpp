//
// Created by josh on 5/21/23.
//

#include "model.h"
#ifdef __GNUC__   // STB image SIMD support doesn't work with gcc
    #define STBI_NO_SIMD
#endif
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_USE_CPP14
#include "renderer.h"
#include <file.h>
#include <meshoptimizer.h>
#include <nlohmann/json.hpp>
#include <tiny_gltf.h>

namespace dragonfire {

static tinygltf::TinyGLTF initGltf();

static void loadGltfFile(const char* path, tinygltf::Model* model, tinygltf::TinyGLTF& gltf)
{
    const char* end = strrchr(path, '.');
    std::string err, warn;
    bool ok;
    if (strcmp(end, ".gltf") == 0)
        ok = gltf.LoadASCIIFromFile(model, &err, &warn, path);
    else if (strcmp(end, ".glb") == 0)
        ok = gltf.LoadBinaryFromFile(model, &err, &warn, path);
    else
        throw FormattedError("File \"{}\" is not a GLTF file", path);

    if (!err.empty())
        spdlog::error("Error loading gltf model \"{}\", error: {}", path, err);
    if (!warn.empty())
        spdlog::warn("Warning in gltf model \"{}\", warning: {}", path, warn);
    if (!ok)
        throw std::runtime_error("Failed to load gltf model");
}

static const unsigned char* getBufferData(const USize index, const tinygltf::Model& model, USize* count = nullptr)
{
    const tinygltf::Accessor& accessor = model.accessors[index];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    if (count)
        *count = accessor.count;
    return &buffer.data[bufferView.byteOffset + accessor.byteOffset];
}

static const unsigned char* getBufferData(
        const std::string& id,
        const tinygltf::Model& model,
        const tinygltf::Primitive& primitive,
        USize* count = nullptr
)
{
    try {
        return getBufferData(primitive.attributes.at(id), model, count);
    }
    catch (const std::out_of_range&) {
        return nullptr;
    }
}

static void optimize(std::vector<Model::Vertex>& vertices, std::vector<UInt32>& indices)
{
    std::vector<UInt32> remap(indices.size());
    USize vertexCount = meshopt_generateVertexRemap(
            remap.data(),
            indices.data(),
            indices.size(),
            vertices.data(),
            indices.size(),
            sizeof(Model::Vertex)
    );
    vertices.resize(vertexCount);
    meshopt_remapIndexBuffer(indices.data(), indices.data(), indices.size(), remap.data());
    meshopt_remapVertexBuffer(vertices.data(), vertices.data(), vertexCount, sizeof(Model::Vertex), remap.data());
    meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertexCount);
    meshopt_optimizeOverdraw(
            indices.data(),
            indices.data(),
            indices.size(),
            &vertices[0].position.x,
            vertexCount,
            sizeof(Model::Vertex),
            1.05f
    );
    meshopt_optimizeVertexFetch(
            vertices.data(),
            indices.data(),
            indices.size(),
            vertices.data(),
            vertexCount,
            sizeof(Model::Vertex)
    );
}

static USize loadIndices(USize accessorIndex, const tinygltf::Model& model, std::vector<UInt32>& indices)
{
    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

    indices.reserve(accessor.count);
    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        const UInt16* ptr = reinterpret_cast<const UInt16*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
        for (USize i = 0; i < accessor.count; i++)
            indices.push_back(ptr[i]);
    }
    else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        const UInt32* ptr = reinterpret_cast<const UInt32*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
        for (USize i = 0; i < accessor.count; i++)
            indices.push_back(ptr[i]);
    }
    else
        spdlog::error("Unknown gltf index type");
    return accessor.count;
}

static Material::TextureWrapMode getWrapMode(int mode)
{
    switch (mode) {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return Material::TextureWrapMode::CLAMP_TO_EDGE;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return Material::TextureWrapMode::MIRRORED_REPEAT;
        case TINYGLTF_TEXTURE_WRAP_REPEAT: return Material::TextureWrapMode::REPEAT;
        default: crash("Unreachable");
    }
}

static Material::TextureFilterMode getFilterMode(int mode)
{
    switch (mode) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST: return Material::TextureFilterMode::NEAREST;
        case TINYGLTF_TEXTURE_FILTER_LINEAR: return Material::TextureFilterMode::LINEAR;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST: return Material::TextureFilterMode::NEAREST_MIPMAP_NEAREST;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST: return Material::TextureFilterMode::LINEAR_MIPMAP_NEAREST;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR: return Material::TextureFilterMode::NEAREST_MIPMAP_LINEAR;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR: return Material::TextureFilterMode::LINEAR_MIPMAP_LINEAR;
        default: return Material::TextureFilterMode::NONE;
    }
}

static UInt32 loadTexture(USize index, const tinygltf::Model& model, Renderer* renderer)
{
    const tinygltf::Texture& texture = model.textures[index];
    const tinygltf::Sampler& sampler = model.samplers[texture.sampler];
    const tinygltf::Image& image = model.images[texture.source];
    Material::TextureWrapMode wrapS, wrapT;
    wrapS = getWrapMode(sampler.wrapS);
    wrapT = getWrapMode(sampler.wrapT);
    Material::TextureFilterMode minFilter, magFilter;
    minFilter = getFilterMode(sampler.minFilter);
    magFilter = getFilterMode(sampler.magFilter);
    const std::string& name = texture.name.empty() ? image.name : texture.name;
    auto id = renderer->loadTexture(
            name,
            image.image.data(),
            image.width,
            image.height,
            image.bits,
            image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? 2 : 1,
            wrapS,
            wrapT,
            minFilter,
            magFilter
    );
    spdlog::info("Loaded texture \"{}\" at index {}", name, id);
    return id;
}

Material loadMaterial(const tinygltf::Material& material, const tinygltf::Model& model, Renderer* renderer)
{
    std::string pipelineId = material.values.contains("PIPELINE_ID") ? material.values.at("PIPELINE_ID").string_value
                                                                     : material.name;
    TextureIds textures{};
    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
        textures.albedo = loadTexture(material.pbrMetallicRoughness.baseColorTexture.index, model, renderer);
    if (material.normalTexture.index >= 0)
        textures.normal = loadTexture(material.normalTexture.index, model, renderer);

    return Material(std::move(pipelineId), textures);
}

static glm::vec4 computeBounds(const std::vector<Model::Vertex>& vertices, const std::vector<UInt32>& indices)
{
    float radius = 0;
    glm::vec3 center(0);
    for (UInt32 index : indices) {
        const Model::Vertex& vertex = vertices[index];
        center += glm::vec3(vertex.position);
    }
    center /= float(indices.size());
    for (UInt32 index : indices) {
        const Model::Vertex& vertex = vertices[index];
        radius = std::max(radius, glm::distance(center, vertex.position));
    }
    return {center, radius};
}

Model Model::loadGltfModel(const char* path, Renderer* renderer, bool optimizeModel)
{
    static thread_local tinygltf::TinyGLTF gltf = initGltf();
    tinygltf::Model model;
    loadGltfFile(path, &model, gltf);
    std::vector<Vertex> vertices;
    std::vector<UInt32> indices;
    Model out;
    for (const tinygltf::Mesh& mesh : model.meshes) {
        for (const tinygltf::Primitive& primitive : mesh.primitives) {
            USize count = 0;
            const float* positions =
                    reinterpret_cast<const float*>(getBufferData("POSITION", model, primitive, &count));
            const float* normals = reinterpret_cast<const float*>(getBufferData("NORMAL", model, primitive));
            const float* uvs = reinterpret_cast<const float*>(getBufferData("TEXCOORD_0", model, primitive));
            vertices.reserve(count);
            for (USize i = 0; i < count; i++) {
                Vertex& vertex = vertices.emplace_back();
                vertex.position.x = positions[i * 3 + 0];
                vertex.position.y = positions[i * 3 + 1];
                vertex.position.z = positions[i * 3 + 2];
                vertex.normal.x = normals[i * 3 + 0];
                vertex.normal.y = normals[i * 3 + 1];
                vertex.normal.z = normals[i * 3 + 2];
                if (uvs) {
                    vertex.uv.x = uvs[i * 2 + 0];
                    vertex.uv.y = uvs[i * 2 + 1];
                }
            }
            USize indexCount = loadIndices(primitive.indices, model, indices);
            if (optimizeModel)
                optimize(vertices, indices);
            MeshHandle handle = renderer->createMesh(vertices, indices);
            Material material = loadMaterial(model.materials[primitive.material], model, renderer);

            glm::vec4 bounds = computeBounds(vertices, indices);
            out.primitives.emplace_back(Primitive{
                    handle,
                    bounds,
                    material,
                    glm::rotate(glm::identity<glm::mat4>(), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            });

            spdlog::info("Loaded primitive geometry with {} vertices and {} indices and radius {}", count, indexCount, bounds.w);
            vertices.clear();
            indices.clear();
        }
    }

    return out;
}

tinygltf::TinyGLTF initGltf()
{
    tinygltf::TinyGLTF loader;
    loader.SetPreserveImageChannels(true);
    tinygltf::FsCallbacks fsCallbacks{};
    fsCallbacks.FileExists = [](const std::string& filename, void*) { return PHYSFS_exists(filename.c_str()) != 0; };
    fsCallbacks.ExpandFilePath = [](const std::string& path, void*) { return path; };
    fsCallbacks.ReadWholeFile = [](std::vector<unsigned char>* out, std::string* err, const std::string& path, void*) {
        try {
            File file(path.c_str());
            *out = file.readData();
        }
        catch (const std::exception& e) {
            *err = e.what();
            return false;
        }
        return true;
    };
    fsCallbacks.WriteWholeFile =
            [](std::string* err, const std::string& path, const std::vector<unsigned char>& data, void*) {
                try {
                    File file(path.c_str());
                    file.writeData(data);
                }
                catch (const std::exception& e) {
                    *err = e.what();
                    return false;
                }
                return true;
            };
    fsCallbacks.GetFileSizeInBytes =
            [](size_t* filesize_out, std::string* err, const std::string& abs_filename, void*) {
                try {
                    File file(abs_filename.c_str());
                    *filesize_out = file.length();
                }
                catch (const std::exception& e) {
                    *err = e.what();
                    return false;
                }
                return true;
            };

    loader.SetFsCallbacks(fsCallbacks);
    return loader;
}
}   // namespace dragonfire