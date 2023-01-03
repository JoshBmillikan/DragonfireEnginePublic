//
// Created by josh on 1/3/23.
//

#include "ui/ui_renderer.h"

namespace df::ui {
CefRefPtr<CefAccessibilityHandler> UIRenderer::GetAccessibilityHandler()
{
    return CefRenderHandler::GetAccessibilityHandler();
}

bool UIRenderer::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    return CefRenderHandler::GetRootScreenRect(browser, rect);
}

void UIRenderer::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
}

bool UIRenderer::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY)
{
    return CefRenderHandler::GetScreenPoint(browser, viewX, viewY, screenX, screenY);
}

bool UIRenderer::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info)
{
    return CefRenderHandler::GetScreenInfo(browser, screen_info);
}

void UIRenderer::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
    CefRenderHandler::OnPopupShow(browser, show);
}

void UIRenderer::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect)
{
    CefRenderHandler::OnPopupSize(browser, rect);
}

void UIRenderer::OnPaint(
        CefRefPtr<CefBrowser> browser,
        CefRenderHandler::PaintElementType type,
        const CefRenderHandler::RectList& dirtyRects,
        const void* buffer,
        int width,
        int height
)
{
}

void UIRenderer::OnAcceleratedPaint(
        CefRefPtr<CefBrowser> browser,
        CefRenderHandler::PaintElementType type,
        const CefRenderHandler::RectList& dirtyRects,
        void* shared_handle
)
{
    CefRenderHandler::OnAcceleratedPaint(browser, type, dirtyRects, shared_handle);
}

bool UIRenderer::StartDragging(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDragData> drag_data,
        CefRenderHandler::DragOperationsMask allowed_ops,
        int x,
        int y
)
{
    return CefRenderHandler::StartDragging(browser, drag_data, allowed_ops, x, y);
}

void UIRenderer::UpdateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation)
{
    CefRenderHandler::UpdateDragCursor(browser, operation);
}

void UIRenderer::OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser, double x, double y)
{
    CefRenderHandler::OnScrollOffsetChanged(browser, x, y);
}

void UIRenderer::OnImeCompositionRangeChanged(
        CefRefPtr<CefBrowser> browser,
        const CefRange& selected_range,
        const CefRenderHandler::RectList& character_bounds
)
{
    CefRenderHandler::OnImeCompositionRangeChanged(browser, selected_range, character_bounds);
}

void UIRenderer::OnTextSelectionChanged(
        CefRefPtr<CefBrowser> browser,
        const CefString& selected_text,
        const CefRange& selected_range
)
{
    CefRenderHandler::OnTextSelectionChanged(browser, selected_text, selected_range);
}

void UIRenderer::OnVirtualKeyboardRequested(CefRefPtr<CefBrowser> browser, CefRenderHandler::TextInputMode input_mode)
{
    CefRenderHandler::OnVirtualKeyboardRequested(browser, input_mode);
}
}   // namespace df::ui