//
// Created by josh on 11/17/22.
//

#pragma once
#include <asset.h>
namespace df {

class Model : public Asset {
    friend class ModelLoader;
    std::unique_ptr<class VertexBuffer> vertexBuffer;

public:
    Model(const std::string& modelName, std::unique_ptr<VertexBuffer>&& vbo);
    static std::unique_ptr<AssetRegistry::Loader> createLoader(class Renderer* renderer);
    VertexBuffer& getVertexBuffer() noexcept {return *vertexBuffer;}
};

}   // namespace df