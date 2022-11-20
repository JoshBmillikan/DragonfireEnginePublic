//
// Created by josh on 11/19/22.
//
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#define TINYOBJLOADER_IMPLEMENTATION
#include "render_asset_loaders.h"
#include "renderer.h"
#include <asset.h>
#include <file.h>
#include <mutex>

namespace df {
std::vector<Asset*> ObjLoader::load(const char* filename)
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
}   // namespace df
