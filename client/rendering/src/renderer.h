//
// Created by josh on 11/9/22.
//

#pragma once
#include "allocation.h"
#include "camera.h"
#include "mesh.h"
#include "model.h"
#include "pipeline.h"
#include "swapchain.h"
#include "vulkan_includes.h"
#include <SDL_video.h>
#include <barrier>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace df {
class Renderer {
    friend class Mesh::Factory;

public:
    Renderer() noexcept = default;
    Renderer(SDL_Window* window);
    void beginRendering(const Camera& camera);
    void render(Model* model, const std::vector<glm::mat4>& matrices);
    void endRendering();
    void stopRendering();
    void destroy() noexcept;
    ~Renderer() noexcept { destroy(); }
    DF_NO_MOVE_COPY(Renderer);
    static constexpr SDL_WindowFlags SDL_WINDOW_FLAGS = SDL_WINDOW_VULKAN;

private:
    static constexpr Size FRAMES_IN_FLIGHT = 2;
    ULong frameCount = 0;
    std::shared_ptr<spdlog::logger> logger;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::Instance instance;
    vk::SurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceLimits limits;
    vk::Device device;
    Swapchain swapchain;
    Image depthImage;
    vk::ImageView depthView;
    PipelineFactory pipelineFactory;
    Buffer globalUniformBuffer;
    vk::DeviceSize globalUniformOffset = 0;
    vk::DescriptorSetLayout globalDescriptorSetLayout;
    vk::DescriptorPool descriptorPool;
    vk::SampleCountFlagBits rasterSamples = vk::SampleCountFlagBits::e1;

    glm::mat4 viewPerspective{};
    glm::mat4 viewOrthographic{};

    std::stop_source threadStop;
    std::mutex presentLock;
    vk::Result presentResult = vk::Result::eSuccess;
    std::condition_variable_any presentCond;
    std::thread presentThreadHandle;

    UInt renderThreadCount = std::max(std::thread::hardware_concurrency() / 2, 1u), currentThread = 0;
    std::barrier<> renderBarrier{renderThreadCount + 1};
    vk::CommandBuffer* secondaryBuffers = nullptr;

    enum class RenderCommand {
        waiting,
        begin,
        render,
        end,
    };

    struct RenderThreadData {
        std::thread thread;
        std::mutex mutex;
        std::condition_variable_any cond;
        RenderCommand command = RenderCommand::waiting;
        const glm::mat4* matrices;
        UInt matrixCount = 0;
        Model* model;
        vk::DescriptorSet uboDescriptorSet;
    }* threadData = nullptr;

    struct Queues {
        UInt graphicsFamily = 0, presentFamily = 0, transferFamily = 0;
        vk::Queue graphics, present, transfer;
        bool getFamilies(vk::PhysicalDevice pDevice, vk::SurfaceKHR surfaceKhr);
    } queues;

    struct Frame {
        vk::CommandPool pool;
        vk::CommandBuffer buffer;
        vk::Fence fence;
        vk::Semaphore renderSemaphore, presentSemaphore;
        vk::DescriptorSet globalDescriptorSet;
    } frames[FRAMES_IN_FLIGHT], *presentingFrame = nullptr;

    struct UboData {
        alignas(16) glm::mat4 viewPerspective;
        alignas(16) glm::mat4 viewOrthographic;
        alignas(16) glm::vec3 sunAngle;
    };

private:
    Frame& getCurrentFrame() noexcept { return frames[frameCount % FRAMES_IN_FLIGHT]; }
    [[nodiscard]] bool depthHasStencil() const noexcept;
    void presentThread(const std::stop_token& token);
    void renderThread(const std::stop_token& token, UInt threadIndex);
    bool cullTest(Model* model, const glm::mat4& matrix);
    void beginCommandRecording();
    void imageToPresentSrc(vk::CommandBuffer cmd) const;
    void imageToColorWrite(vk::CommandBuffer cmd) const;
    void recreateSwapchain();
    void init(SDL_Window* window, bool validation);
    void createInstance(SDL_Window* window, bool validation);
    void getPhysicalDevice();
    void createDevice();
    void initAllocator() const;
    void createDepthImage();
    void allocateUniformBuffer();
    void createDescriptorPool();
    void createFrames();

public:
    PipelineFactory* getPipelineFactory() noexcept { return &pipelineFactory; }
    [[nodiscard]] vk::Device getDevice() const noexcept { return device; };
    [[nodiscard]] vk::DescriptorSetLayout getGlobalDescriptorSetLayout() const noexcept { return globalDescriptorSetLayout; }
};

}   // namespace df