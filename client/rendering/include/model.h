//
// Created by josh on 11/17/22.
//

#pragma once
namespace df {

class Model {
    class Mesh* mesh;
    class Material* material;

public:
    Model(const char* meshId, const char* materialId);
    Mesh& getMesh() noexcept {return *mesh;}
    Material& getMaterial() noexcept {return *material;}
};

}   // namespace df