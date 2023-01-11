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
#include "texture.h"
#include "vulkan_includes.h"
#include <SDL_video.h>
#include <barrier>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace df {
class Renderer {
    friend class Swapchain;
    friend class Mesh::Factory;
    friend class Texture::Factory;
    friend class VulkanUIRenderer;

public:
    /**
     *  Default constructs the class with zero init fields
     */
    Renderer() noexcept = default;

    /**
     * Creates and initializes a new renderer
     * @param window SDL window handle
     */
    Renderer(SDL_Window* window);

    /**
     * Begin rendering a the next frame
     * @param camera camera to use for the view and projection matrices
     */
    void beginRendering(const Camera& camera);

    /**
     * Dispatch a model and set of matrices to one of the worker threads
     * @param model the model to render
     * @param matrices the matrices for each model instance
     */
    void render(Model* model, const std::vector<glm::mat4>& matrices);

    /**
     * End rendering the current frame and present it
     */
    void endRendering();

    /**
     * Join all rendering threads and wait for the queues to be idle
     */
    void stopRendering();

    /**
     * Destroys the renderer and all associated resources
     */
    void destroy() noexcept;

    ~Renderer() noexcept { destroy(); }
    DF_NO_MOVE_COPY(Renderer);
    static constexpr SDL_WindowFlags SDL_WINDOW_FLAGS = SDL_WINDOW_VULKAN;

private:
    static constexpr Size FRAMES_IN_FLIGHT = 2;
    static constexpr UInt MAX_TEXTURE_DESCRIPTORS = 4096;
    ULong frameCount = 0;
    std::shared_ptr<spdlog::logger> logger;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::Instance instance;
    vk::SurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceLimits limits;
    vk::Device device;
    vk::RenderPass mainPass;
    Swapchain swapchain;
    Image depthImage, msaaImage;
    vk::ImageView depthView, msaaView;
    PipelineFactory pipelineFactory;
    Buffer globalUniformBuffer;
    vk::DeviceSize globalUniformOffset = 0;
    vk::DescriptorSetLayout globalDescriptorSetLayout;
    vk::DescriptorPool descriptorPool;
    vk::SampleCountFlagBits rasterSamples = vk::SampleCountFlagBits::e1;
    glm::mat4 viewPerspective{}, viewOrthographic{};

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
    /// Gets a reference to the current frame
    Frame& getCurrentFrame() noexcept { return frames[frameCount % FRAMES_IN_FLIGHT]; }
    /// returns if the depth format has a stencil component
    [[nodiscard]] bool depthHasStencil() const noexcept;
    /// The presentation thread function
    void presentThread(const std::stop_token& token);
    /// The render worker thread function
    void renderThread(const std::stop_token& token, UInt threadIndex);
    /// Returns true if the model should be rendered with the given transform
    bool cullTest(Model* model, const glm::mat4& matrix);
    /// Start recording a command buffer
    void beginCommandRecording();
    /// Recreate the swapchain and associated data
    void recreateSwapchain();
    /// Initialize the renderer
    void init(SDL_Window* window, bool validation);
    /// Create the vulkan instance
    void createInstance(SDL_Window* window, bool validation);
    /// Get the gpu handle to use
    void getPhysicalDevice();
    /// Get the msaa sample count
    void getSampleCount();
    /// Create the vulkan device handle
    void createDevice();
    /// Creates the main render pass
    void createRenderPass();
    /// Initialize the VMA allocator
    void initAllocator() const;
    /// Create the depth image and view
    void createDepthImage();
    /// Create the buffer for MSAA sampling
    void createMsaaImage();
    /// Allocate the global uniform buffer
    void allocateUniformBuffer();
    /// Create the descriptor pool and layout
    void createDescriptorPool();
    /// Create per frame data
    void createFrames();

public:
    PipelineFactory* getPipelineFactory() noexcept { return &pipelineFactory; }
    [[nodiscard]] vk::Device getDevice() const noexcept { return device; };
    [[nodiscard]] vk::DescriptorSetLayout getGlobalDescriptorSetLayout() const noexcept { return globalDescriptorSetLayout; }
};

}   // namespace df