//
// Created by josh on 5/21/23.
//

#include "material.h"
#include <nlohmann/json.hpp>
#include <utility.h>

namespace dragonfire {

bool Material::ShaderEffect::operator==(const Material::ShaderEffect& other) const
{
    return renderPassIndex == other.renderPassIndex && subpass == other.subpass && enableDepth == other.enableDepth
           && enableMultisampling == other.enableMultisampling && enableColorBlend == other.enableColorBlend
           && shaderNames.vertex == other.shaderNames.vertex && shaderNames.fragment == other.shaderNames.fragment
           && shaderNames.geometry == other.shaderNames.geometry && shaderNames.tessEval == other.shaderNames.tessEval
           && shaderNames.tessCtrl == other.shaderNames.tessCtrl && topology == other.topology;
}

USize Material::ShaderEffect::Hash::operator()(const Material::ShaderEffect& effect) const
{
    return hashAll(
            effect.renderPassIndex,
            effect.subpass,
            effect.enableDepth,
            effect.enableMultisampling,
            effect.enableColorBlend,
            effect.shaderNames.vertex,
            effect.shaderNames.vertex,
            effect.shaderNames.fragment,
            effect.shaderNames.geometry,
            effect.shaderNames.tessEval,
            effect.shaderNames.tessCtrl,
            effect.topology
    );
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
        Material::ShaderEffect::ShaderNames,
        vertex,
        fragment,
        geometry,
        tessEval,
        tessCtrl
);

NLOHMANN_JSON_SERIALIZE_ENUM(
        Material::ShaderEffect::Topology,
        {{Material::ShaderEffect::Topology::triangleList, "triangleList"},
         {Material::ShaderEffect::Topology::triangleFan, "triangleFan"},
         {Material::ShaderEffect::Topology::point, "point"},
         {Material::ShaderEffect::Topology::list, "list"}}
);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
        Material::ShaderEffect,
        renderPassIndex,
        subpass,
        enableDepth,
        enableMultisampling,
        enableColorBlend,
        shaderNames,
        topology
);

Material::ShaderEffect::ShaderEffect(const nlohmann::json& json)
{
    json.get_to(*this);
}

}   // namespace dragonfire