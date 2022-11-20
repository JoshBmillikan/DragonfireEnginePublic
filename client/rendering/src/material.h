//
// Created by josh on 11/19/22.
//

#pragma once
#include "vulkan_includes.h"
#include "pipeline.h"
#include <asset.h>

namespace df {

class Material : public Asset {
    friend class MaterialLoader;
    vk::Pipeline pipeline;
    vk::PipelineLayout layout;
    vk::Device device;
public:
    Material() = default;
    ~Material() override;
    DF_NO_MOVE_COPY(Material);
};

}   // namespace df