//
// Created by josh on 5/21/23.
//

#pragma once
#include <nlohmann/json_fwd.hpp>

namespace dragonfire {

class Material {
public:
    struct ShaderEffect {
        uint32_t renderPassIndex = 0;
        uint32_t subpass = 0;
        bool enableDepth = true;
        bool enableMultisampling = true;
        bool enableColorBlend = false;

        struct {
            std::string vertex;
            std::string fragment;
            std::string geometry;
            std::string tessEval;
            std::string tessCtrl;
        } shaderNames;
        enum class Topology {
            triangleList,
            triangleFan,
            point,
            list,
        } topology = Topology::triangleList;

        struct Hash {
            USize operator()(const ShaderEffect& effect) const;
        };

        bool operator==(const ShaderEffect& other) const;

        ShaderEffect(const nlohmann::json& json);
        ShaderEffect() = default;
    };

    struct Library {
        virtual ~Library() = default;
        virtual Material* getMaterial(const std::string& name) = 0;
        virtual void loadMaterialFiles(const char* dir, class Renderer* renderer) = 0;
    };
};

}   // namespace dragonfire