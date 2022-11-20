//
// Created by josh on 11/19/22.
//

#pragma once
#include "asset.h"
#include "mesh.h"
#include "model.h"
#include "tiny_obj_loader.h"
namespace df {

class ObjLoader : public AssetRegistry::Loader {
public:
    std::vector<Asset*> load(const char* filename) override;
    ObjLoader(Renderer* renderer) : factory(renderer) {}

private:
    Mesh::Factory factory;
    std::mutex mutex;
    std::vector<Vertex> vertices;
    std::vector<UInt> indices;
    HashMap<Vertex, UInt> uniqueVertices;

    Mesh* createMesh(const tinyobj::shape_t& shape, const tinyobj::attrib_t& attributes);
};
}