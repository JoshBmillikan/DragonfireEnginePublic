//
// Created by josh on 11/17/22.
//

#include "model.h"
#include "vertex_buffer.h"
#include <file.h>
#include <mutex>
#include "renderer.h"
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace df {
class ModelLoader : public AssetRegistry::Loader {
public:
    Asset* load(const char* filename) override
    {
        tinyobj::ObjReader reader;
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
        auto& materials = reader.GetMaterials();
        if (shapes.size() > 1)
            logger->warn("More than one model found in OBJ file {}, only the first will be loaded!", filename);

        auto vbo = createVertexBuffer(shapes[0], attributes);
        return new Model(shapes[0].name, std::move(vbo));
    }
    ModelLoader(Renderer* renderer) : factory(renderer) {}

private:
    VertexBuffer::Factory factory;
    std::mutex mutex;
    std::vector<Vertex> vertices;
    std::vector<UInt> indices;
    HashMap<Vertex, UInt> uniqueVertices;
    std::unique_ptr<VertexBuffer> createVertexBuffer(const tinyobj::shape_t& shape, const tinyobj::attrib_t& attributes)
    {
        std::unique_lock lock(mutex);
        vertices.clear();
        indices.clear();
        uniqueVertices.clear();
        vertices.reserve(shape.mesh.num_face_vertices.size());
        indices.reserve(shape.mesh.indices.size());
        uniqueVertices.reserve(shape.mesh.indices.size());
        for (const auto& index : shape.mesh.indices) {
            Vertex v{
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
            if (!uniqueVertices.contains(v)) {
                uniqueVertices[v] = vertices.size();
                vertices.emplace_back(v);
            }
            indices.emplace_back(uniqueVertices[v]);
        }
        logger->info("Loaded model \"{}\" with {} vertices", shape.name, vertices.size());
        return std::unique_ptr<VertexBuffer>(factory.create(vertices, indices));
    }
};

Model::Model(const std::string& modelName, std::unique_ptr<VertexBuffer>&& vbo) : vertexBuffer(std::move(vbo))
{
    name = modelName;
}

std::unique_ptr<AssetRegistry::Loader> Model::createLoader(Renderer* renderer)
{
    return std::make_unique<ModelLoader>(renderer);
}
}   // namespace df