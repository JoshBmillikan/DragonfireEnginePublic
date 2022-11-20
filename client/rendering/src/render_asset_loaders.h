//
// Created by josh on 11/19/22.
//

#pragma once
#include "asset.h"
#include "mesh.h"
#include "material.h"
#include "pipeline.h"
#include "tiny_obj_loader.h"
#include <nlohmann/json_fwd.hpp>

namespace df {

class ObjLoader : public AssetRegistry::Loader {
public:
    std::vector<Asset*> load(const char* filename) override;
    explicit ObjLoader(Renderer* renderer) : factory(renderer) {}

private:
    Mesh::Factory factory;
    std::mutex mutex;
    std::vector<Vertex> vertices;
    std::vector<UInt> indices;
    HashMap<Vertex, UInt> uniqueVertices;
    tinyobj::ObjReader reader;

    Mesh* createMesh(const tinyobj::shape_t& shape, const tinyobj::attrib_t& attributes);
};

class MaterialLoader : public AssetRegistry::Loader {
    PipelineFactory* pipelineFactory;
    vk::Device device;
    vk::DescriptorSetLayout setLayout;
public:
    MaterialLoader(PipelineFactory* pipelineFactory, vk::Device device, vk::DescriptorSetLayout setLayout);
    std::vector<Asset*> load(const char* filename) override;
    Material* createMaterial(nlohmann::json& json);
    vk::PipelineLayout createPipelineLayout(nlohmann::json& json, vk::Device device);
};
}