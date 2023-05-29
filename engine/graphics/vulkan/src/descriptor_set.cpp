//
// Created by josh on 5/22/23.
//

#include "descriptor_set.h"
#include "allocators.h"
#include <vulkan/vulkan_hash.hpp>

namespace dragonfire {

vk::DescriptorSetLayout DescriptorLayoutManager::getOrCreateLayout(DescriptorLayoutManager::LayoutInfo& info)
{
    std::unique_lock lock(mutex);
    if (createdLayouts.contains(info))
        return createdLayouts[info];

    std::sort(info.bindings.begin(), info.bindings.end(), [](const auto& a, const auto& b) {
        return a.binding < b.binding;
    });
    if (info.bindings.size() > 1) {
        for (UInt i = 0; i < info.bindings.size() - 1; i++) {
            auto& a = info.bindings[i];
            auto& b = info.bindings[i + 1];
            if (a.binding == b.binding) {
                a.stageFlags |= b.stageFlags;
                b.stageFlags |= a.stageFlags;
            }
        }
    }

    info.bindings.erase(std::unique(info.bindings.begin(), info.bindings.end()), info.bindings.end());
    vk::DescriptorSetLayoutCreateInfo createInfo{};
    createInfo.bindingCount = info.bindings.size();
    createInfo.pBindings = info.bindings.data();
    createInfo.flags = info.flags;

    vk::DescriptorSetLayout layout;
    if (info.bindless) {
        std::vector<vk::DescriptorBindingFlags, FrameAllocator<vk::DescriptorBindingFlags>> flags;
        flags.resize(info.bindings.size());
        flags.back() |= vk::DescriptorBindingFlagBits::ePartiallyBound
                        | vk::DescriptorBindingFlagBits::eVariableDescriptorCount
                        | vk::DescriptorBindingFlagBits::eUpdateAfterBind;
        vk::DescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo{};
        flagsCreateInfo.bindingCount = flags.size();
        flagsCreateInfo.pBindingFlags = flags.data();
        createInfo.pNext = &flagsCreateInfo;
        createInfo.flags |= vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
        layout = device.createDescriptorSetLayout(createInfo);
    }
    else
        layout = device.createDescriptorSetLayout(createInfo);
    createdLayouts[info] = layout;
    return layout;
}

void DescriptorLayoutManager::destroy()
{
    if (device) {
        for (auto& [info, layout] : createdLayouts)
            device.destroy(layout);
        device = nullptr;
    }
}

DescriptorLayoutManager::DescriptorLayoutManager(DescriptorLayoutManager&& other) noexcept
{
    if (this != &other) {
        std::unique_lock a(mutex), b(other.mutex);
        destroy();
        device = other.device;
        createdLayouts = std::move(other.createdLayouts);
    }
}

DescriptorLayoutManager& DescriptorLayoutManager::operator=(DescriptorLayoutManager&& other) noexcept
{
    if (this != &other) {
        std::unique_lock a(mutex), b(other.mutex);
        destroy();
        device = other.device;
        createdLayouts = std::move(other.createdLayouts);
    }
    return *this;
}

USize DescriptorLayoutManager::LayoutInfo::Hash::operator()(const DescriptorLayoutManager::LayoutInfo& info) const
{
    USize hash = std::hash<vk::DescriptorSetLayoutCreateFlags>()(info.flags);
    hash ^= std::hash<bool>()(info.bindless);
    for (auto& binding : info.bindings)
        hash ^= std::hash<vk::DescriptorSetLayoutBinding>()(binding);
    return hash;
}

bool DescriptorLayoutManager::LayoutInfo::operator==(const DescriptorLayoutManager::LayoutInfo& other) const
{
    return bindings == other.bindings && flags == other.flags && bindless == other.bindless;
}
}   // namespace dragonfire