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
            const float* uvs = reinterpret_cast<const float*>(getBufferData("UV", model, primitive));
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
            const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
            const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

            indices.reserve(accessor.count);
            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const UInt16* ptr =
                        reinterpret_cast<const UInt16*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (USize i = 0; i < accessor.count; i++)
                    indices.push_back(ptr[i]);
            }
            else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                const UInt32* ptr =
                        reinterpret_cast<const UInt32*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (USize i = 0; i < accessor.count; i++)
                    indices.push_back(ptr[i]);
            }
            else
                spdlog::error("Unknown gltf index type");
            if (optimizeModel)
                optimize(vertices, indices);
            MeshHandle handle = renderer->createMesh(vertices, indices);
            Material* material = renderer->getMaterialLibrary()->getMaterial("basic");   // TODO get from gltf
            out.primitives.emplace_back(Primitive{handle, material, glm::identity<glm::mat4>()});

            spdlog::info("Loaded primitive geometry with {} vertices and {} indices", count, accessor.count);
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