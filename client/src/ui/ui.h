//
// Created by josh on 12/30/22.
//

#pragma once
#include "application.h"
#include <ui/ui_renderer.h>

#include <utility>

namespace df::ui {
CefRefPtr<Application> initCEF(int argc, char** argv);
CefRefPtr<CefBrowser> createBrowser(const CefRefPtr<CefClient>& client);

class UI : public CefClient, public CefLifeSpanHandler, public CefLoadHandler {
    IMPLEMENT_REFCOUNTING(UI);

public:
    UI(CefRefPtr<CefRenderHandler> renderer) : renderer(std::move(renderer)) {}

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }

    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }

    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return renderer; }

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;

private:
    CefRefPtr<CefRenderHandler> renderer;
    int browserId = 0;
};
}   // namespace df::ui