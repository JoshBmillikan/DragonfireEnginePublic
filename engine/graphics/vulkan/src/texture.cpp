//
// Created by josh on 5/29/23.
//

#include "texture.h"
#if defined(_MSC_VER) || defined(__MINGW32__)
    #include <malloc.h>
#else
    #include <alloca.h>
#endif

namespace dragonfire {

static vk::Filter getFilter(Material::TextureFilterMode mode)
{
    switch (mode) {
        default: return vk::Filter::eLinear;
        case Material::TextureFilterMode::NEAREST: return vk::Filter::eNearest;
    }
}

static vk::SamplerAddressMode getAddressMode(Material::TextureWrapMode mode)
{
    switch (mode) {
        case Material::TextureWrapMode::CLAMP_TO_EDGE: return vk::SamplerAddressMode::eClampToEdge;
        case Material::TextureWrapMode::MIRRORED_REPEAT: return vk::SamplerAddressMode::eMirroredRepeat;
        default:
        case Material::TextureWrapMode::REPEAT: return vk::SamplerAddressMode::eRepeat;
    }
}

UInt32 Texture::TextureRegistry::loadTexture(
        const std::string& name,
        const void* data,
        UInt32 width,
        UInt32 height,
        UInt bitDepth,
        UInt pixelSize,
        Material::TextureWrapMode wrapS,
        Material::TextureWrapMode wrapT,
        Material::TextureFilterMode minFilter,
        Material::TextureFilterMode magFilter
)
{
    const USize size = width * height * pixelSize * 4;
    std::unique_lock lock(mutex);
    if (textures.contains(name))
        return textures[name].id;
    Image imageTexture =
            Image::Builder()
                    .withAllocator(allocator)
                    .withSharingMode(vk::SharingMode::eExclusive)
                    .withExtent(vk::Extent2D{width, height})
                    .withImageType(vk::ImageType::e2D)
                    .withImageUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
                    .withMipLevels(1)
                    .withArrayLayers(1)
                    .withFormat(vk::Format::eR8G8B8A8Srgb)
                    .withTiling(vk::ImageTiling::eOptimal)
                    .withInitialLayout(vk::ImageLayout::eUndefined)
                    .withSamples(vk::SampleCountFlagBits::e1)
                    .withRequiredFlags(vk::MemoryPropertyFlagBits::eDeviceLocal)
                    .withUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                    .build();
    void* ptr = getStagingPtr(size);
    memcpy(ptr, data, size);

    device.resetCommandPool(pool);
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(beginInfo);

    vk::ImageMemoryBarrier barrier{};
    barrier.image = imageTexture;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstQueueFamilyIndex = barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

    vk::BufferImageCopy cpy;
    cpy.bufferOffset = 0;
    cpy.bufferRowLength = 0;
    cpy.bufferImageHeight = 0;
    cpy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    cpy.imageSubresource.mipLevel = 0;
    cpy.imageSubresource.baseArrayLayer = 0;
    cpy.imageSubresource.layerCount = 1;
    cpy.imageOffset = vk::Offset3D{0, 0, 0};
    cpy.imageExtent = vk::Extent3D{width, height, 1};

    cmd.copyBufferToImage(stagingBuffer, imageTexture, vk::ImageLayout::eTransferDstOptimal, cpy);

    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            {},
            {},
            barrier
    );

    cmd.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    graphicsQueue.submit(submitInfo, fence);

    vk::resultCheck(device.waitForFences(fence, true, UINT64_MAX), "Fence wait failed");
    UInt32 imageId = imageIndex++;

    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = getFilter(magFilter);
    samplerInfo.minFilter = getFilter(minFilter);
    samplerInfo.addressModeU = getAddressMode(wrapS);
    samplerInfo.addressModeV = getAddressMode(wrapT);
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable = true;
    samplerInfo.maxAnisotropy = maxSamplerAnisotropy;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = false;
    samplerInfo.compareEnable = false;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = 0;

    vk::Sampler s = device.createSampler(samplerInfo);

    textures[name] = Texture(imageId, std::move(imageTexture), s, device);

    return imageId;
}

Texture::Texture(UInt32 id, Image&& image, vk::Sampler sampler, vk::Device device)
    : id(id), sampler(sampler), image(std::move(image))
{
    vk::ImageSubresourceRange subRange{};
    subRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    subRange.baseMipLevel = 0;
    subRange.levelCount = 1;
    subRange.baseArrayLayer = 0;
    subRange.layerCount = 1;
    view = this->image.createView(device, subRange);
}

Texture::TextureRegistry::TextureRegistry(
        vk::Device device,
        VmaAllocator allocator,
        vk::Queue graphicsQueue,
        UInt32 graphicsFamily,
        float maxSamplerAnisotropy
)
    : allocator(allocator), device(device), graphicsQueue(graphicsQueue), maxSamplerAnisotropy(maxSamplerAnisotropy)
{
    stagingBuffer = Buffer::Builder()
                            .withAllocator(allocator)
                            .withSize(1 << 12)
                            .withSharingMode(vk::SharingMode::eExclusive)
                            .withBufferUsage(vk::BufferUsageFlagBits::eTransferSrc)
                            .withUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                            .withAllocationFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                            .withRequiredFlags(
                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
                            )
                            .build();
    vk::CommandPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.queueFamilyIndex = graphicsFamily;
    pool = device.createCommandPool(poolCreateInfo);
    vk::CommandBufferAllocateInfo allocateInfo{};
    allocateInfo.commandBufferCount = 1;
    allocateInfo.level = vk::CommandBufferLevel::ePrimary;
    allocateInfo.commandPool = pool;
    vk::resultCheck(device.allocateCommandBuffers(&allocateInfo, &cmd), "Failed to allocate command buffer");
    fence = device.createFence(vk::FenceCreateInfo());
}

void* Texture::TextureRegistry::getStagingPtr(USize size)
{
    if (stagingBuffer.getInfo().size <= size) {
        stagingBuffer =
                Buffer::Builder()
                        .withSize(std::max(size + 1, stagingBuffer.getInfo().size * 2))
                        .withSharingMode(vk::SharingMode::eExclusive)
                        .withBufferUsage(vk::BufferUsageFlagBits::eTransferSrc)
                        .withAllocator(allocator)
                        .withUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                        .withAllocationFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                        .withRequiredFlags(
                                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
                        )
                        .build();
    }
    return stagingBuffer.getInfo().pMappedData;
}

void Texture::TextureRegistry::destroy() noexcept
{
    if (device) {
        for (auto& [name, texture] : textures) {
            device.destroy(texture.sampler);
            device.destroy(texture.view);
            texture.image.destroy();
        }
        textures.clear();
        device.destroy(pool);
        device.destroy(fence);
        stagingBuffer.destroy();
        device = nullptr;
    }
}

Texture::TextureRegistry::TextureRegistry(Texture::TextureRegistry&& other) noexcept
{
    if (this != &other) {
        std::unique_lock l1(mutex), l2(other.mutex);
        destroy();
        device = other.device;
        other.device = nullptr;
        allocator = other.allocator;
        textures = std::move(other.textures);
        stagingBuffer = std::move(other.stagingBuffer);
        pool = other.pool;
        cmd = other.cmd;
        graphicsQueue = other.graphicsQueue;
        fence = other.fence;
        imageIndex = other.imageIndex;
        maxSamplerAnisotropy = other.maxSamplerAnisotropy;
    }
}

Texture::TextureRegistry& Texture::TextureRegistry::operator=(Texture::TextureRegistry&& other) noexcept
{
    if (this != &other) {
        std::unique_lock l1(mutex), l2(other.mutex);
        destroy();
        device = other.device;
        other.device = nullptr;
        allocator = other.allocator;
        textures = std::move(other.textures);
        stagingBuffer = std::move(other.stagingBuffer);
        pool = other.pool;
        cmd = other.cmd;
        graphicsQueue = other.graphicsQueue;
        fence = other.fence;
        imageIndex = other.imageIndex;
        maxSamplerAnisotropy = other.maxSamplerAnisotropy;
    }
    return *this;
}

void Texture::TextureRegistry::writeDescriptor(const std::string& textureId, vk::DescriptorSet set, UInt32 binding)
{
    Texture& texture = textures.at(textureId);
    vk::DescriptorImageInfo imageInfo{};
    imageInfo.sampler = texture.sampler;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = texture.view;
    vk::WriteDescriptorSet write{};
    write.descriptorCount = 1;
    write.dstArrayElement = texture.id;
    write.dstSet = set;
    write.dstBinding = binding;
    write.pImageInfo = &imageInfo;
    write.descriptorType = vk::DescriptorType::eCombinedImageSampler;

    device.updateDescriptorSets(write, {});
}

}   // namespace dragonfire