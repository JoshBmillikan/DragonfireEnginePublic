//
// Created by josh on 5/21/23.
//

#pragma once
#include <nlohmann/json_fwd.hpp>

namespace dragonfire {

struct TextureIds {
    UInt32 albedo = 0;
    UInt32 normal = 0;
    UInt32 ambient = 0;
    UInt32 diffuse = 0;
    UInt32 specular = 0;
};

class Material {
    TextureIds textureIds;
    std::string pipelineId;

public:
    explicit Material(std::string&& pipelineId = "basic", TextureIds textures = {})
        : textureIds(textures), pipelineId(std::move(pipelineId))
    {
    }

    enum class TextureWrapMode { CLAMP_TO_EDGE, MIRRORED_REPEAT, REPEAT };
    enum class TextureFilterMode {
        NONE,
        NEAREST,
        LINEAR,
        NEAREST_MIPMAP_NEAREST,
        LINEAR_MIPMAP_NEAREST,
        NEAREST_MIPMAP_LINEAR,
        LINEAR_MIPMAP_LINEAR
    };

    struct ShaderEffect {
        uint32_t renderPassIndex = 0;
        uint32_t subpass = 0;
        bool enableDepth = true;
        bool enableMultisampling = true;
        bool enableColorBlend = false;

        struct ShaderNames {
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

    [[nodiscard]] const TextureIds& getTextureIds() const { return textureIds; }

    [[nodiscard]] const std::string& getPipelineId() const { return pipelineId; }

    void setAlbedo(UInt32 index) { textureIds.albedo = index; }

    void setNormal(UInt32 index) { textureIds.normal = index; }

    struct Library {
        virtual ~Library() = default;
        virtual Material* getMaterial(const std::string& name) = 0;
    };
};

}   // namespace dragonfire