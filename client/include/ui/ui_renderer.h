//
// Created by josh on 1/3/23.
//

#pragma once

#include "cef_base.h"
#include "cef_render_handler.h"

namespace df::ui {

class UIRenderer : public CefRenderHandler {
    IMPLEMENT_REFCOUNTING(UIRenderer);
public:
    ~UIRenderer() override = default;
    CefRefPtr<CefAccessibilityHandler> GetAccessibilityHandler() override;
    bool GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    bool GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY) override;
    bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) override;
    void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) override;
    void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) override;
    void OnPaint(
            CefRefPtr<CefBrowser> browser,
            PaintElementType type,
            const RectList& dirtyRects,
            const void* buffer,
            int width,
            int height
    ) override;
    void OnAcceleratedPaint(
            CefRefPtr<CefBrowser> browser,
            PaintElementType type,
            const RectList& dirtyRects,
            void* shared_handle
    ) override;
    bool StartDragging(
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefDragData> drag_data,
            DragOperationsMask allowed_ops,
            int x,
            int y
    ) override;
    void UpdateDragCursor(CefRefPtr<CefBrowser> browser, DragOperation operation) override;
    void OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser, double x, double y) override;
    void OnImeCompositionRangeChanged(
            CefRefPtr<CefBrowser> browser,
            const CefRange& selected_range,
            const RectList& character_bounds
    ) override;
    void OnTextSelectionChanged(CefRefPtr<CefBrowser> browser, const CefString& selected_text, const CefRange& selected_range)
            override;
    void OnVirtualKeyboardRequested(CefRefPtr<CefBrowser> browser, TextInputMode input_mode) override;
protected:

};
}   // namespace df::ui