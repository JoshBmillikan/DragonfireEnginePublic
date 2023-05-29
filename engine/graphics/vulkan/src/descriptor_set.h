//
// Created by josh on 5/22/23.
//

#pragma once
#include <ankerl/unordered_dense.h>

struct SpvReflectShaderModule;

namespace dragonfire {

class DescriptorLayoutManager {
public:
    explicit DescriptorLayoutManager(vk::Device device) : device(device) {}

    DescriptorLayoutManager() = default;

    struct LayoutInfo {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        vk::DescriptorSetLayoutCreateFlagBits flags{};
        bool bindless = false;

        struct Hash {
            USize operator()(const LayoutInfo& info) const;
        };

        bool operator==(const LayoutInfo& other) const;
    };

    vk::DescriptorSetLayout getOrCreateLayout(LayoutInfo& info);

    void destroy();

    ~DescriptorLayoutManager() { destroy(); }

    DescriptorLayoutManager(DescriptorLayoutManager&) = delete;
    DescriptorLayoutManager& operator=(DescriptorLayoutManager&) = delete;
    DescriptorLayoutManager(DescriptorLayoutManager&& other) noexcept;
    DescriptorLayoutManager& operator=(DescriptorLayoutManager&& other) noexcept;

private:
    vk::Device device;
    std::mutex mutex;
    ankerl::unordered_dense::map<LayoutInfo, vk::DescriptorSetLayout, LayoutInfo::Hash> createdLayouts;
};

class DescriptorAllocator {};

}   // namespace dragonfire