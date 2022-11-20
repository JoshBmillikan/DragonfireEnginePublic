//
// Created by josh on 11/19/22.
//

#pragma once
#include "vulkan_includes.h"
#include "pipeline.h"
#include <asset.h>

namespace df {

class Material : public Asset {
    vk::Pipeline pipeline;
    vk::PipelineLayout layout;
public:
    class Loader : public AssetRegistry::Loader {
        PipelineFactory pipelineFactory;
    public:

        std::vector<Asset*> load(const char* filename) override;
    };
};

}   // namespace df