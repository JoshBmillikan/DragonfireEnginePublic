//
// Created by josh on 1/3/23.
//

#pragma once
#include "allocation.h"
#include <ui/ui_renderer.h>

namespace df {

class VulkanUIRenderer : public ui::UIRenderer {
public:
    explicit VulkanUIRenderer(class Renderer* renderer);
    ~VulkanUIRenderer() override;
    DF_NO_MOVE_COPY(VulkanUIRenderer);

    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;

    void OnPaint(
            CefRefPtr<CefBrowser> browser,
            PaintElementType type,
            const RectList& dirtyRects,
            const void* buffer,
            int width,
            int height
    ) override;

    vk::CommandBuffer getCmdBuffer() noexcept
    {
        vk::CommandBuffer buf = needsUpdate ? transferCmds : uiCmds;
        needsUpdate = false;
        return buf;
    };

private:
    vk::CommandPool pool;
    vk::CommandBuffer uiCmds, transferCmds;
    Buffer uiBuffer;
    Image uiImage;
    vk::ImageView uiView;
    vk::Device device;
    vk::Extent2D extent;
    bool needsUpdate = true;

    void createImageBuffers();
};

}   // namespace df