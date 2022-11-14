//
// Created by josh on 11/13/22.
//

#pragma once
#include "vulkan_includes.h"
namespace df {

class PipelineFactory {
    vk::Device device;
    vk::PipelineCache cache;
};

}   // namespace df