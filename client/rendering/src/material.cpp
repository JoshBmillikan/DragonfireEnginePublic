//
// Created by josh on 11/19/22.
//

#include "material.h"

namespace df {

Material::~Material()
{
    device.destroy(pipeline);
    device.destroy(layout);
}

void Material::pushTextureIndices(vk::CommandBuffer cmd, UInt offset)
{
    UInt values[TEXTURE_COUNT];
    values[0] = albedo ? albedo->getIndex() : 0;
    values[1] = roughness ? roughness->getIndex() : 0;
    values[2] = emissive ? emissive->getIndex() : 0;
    values[3] = normal ? normal->getIndex() : 0;
    cmd.pushConstants(layout, vk::ShaderStageFlagBits::eFragment, offset, sizeof(UInt) * TEXTURE_COUNT, values);
}
}   // namespace df