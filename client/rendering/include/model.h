//
// Created by josh on 11/17/22.
//

#pragma once
namespace df {

class Mesh;
class Material;

class Model {
    friend class std::hash<df::Model>;
    Mesh* mesh;
    Material* material;

public:
    Model(const char* meshId, const char* materialId);
    Mesh& getMesh() noexcept { return *mesh; }
    Material& getMaterial() noexcept { return *material; }

    bool operator==(const Model& other) const noexcept { return mesh == other.mesh && material == other.material; }
};

}   // namespace df

namespace std {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCInconsistentNamingInspection"
template<>
struct [[maybe_unused]] hash<df::Model> {
    size_t operator()(df::Model const& model) const
    {
        return ((hash<df::Mesh*>()(model.mesh) ^ (hash<df::Material*>()(model.material) << 1)) >> 1);
    }
};
#pragma clang diagnostic pop
}   // namespace std