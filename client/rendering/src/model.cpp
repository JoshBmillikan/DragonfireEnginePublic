//
// Created by josh on 11/17/22.
//

#include "model.h"
#include <asset.h>
#include "mesh.h"
#include "material.h"
namespace df {

df::Model::Model(const char* meshId, const char* materialId)
{
    auto& registry = AssetRegistry::getRegistry();
    mesh = registry.get<Mesh>(meshId);
    material = registry.get<Material>(materialId);
}
}   // namespace df