//
// Created by josh on 1/3/23.
//

#pragma once
#include <ui/ui_renderer.h>
#include "allocation.h"

namespace df {

class VulkanUIRenderer : public ui::UIRenderer {
    Buffer buffer;
    Image uiImage;
    vk::ImageView uiView;
public:
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    void OnPaint(
            CefRefPtr<CefBrowser> browser,
            PaintElementType type,
            const RectList& dirtyRects,
            const void* buffer,
            int width,
            int height
    ) override;

};

}   // namespace df