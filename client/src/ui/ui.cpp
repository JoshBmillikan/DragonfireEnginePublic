//
// Created by josh on 12/30/22.
//

#include "ui.h"
#include "application.h"
#include "wrapper/cef_helpers.h"
#ifdef _WIN32
    #include <windows.h>
#endif

namespace df::ui {
CefRefPtr<Application> initCEF(int argc, char** argv)
{
#ifdef _WIN32
    CefMainArgs args(GetModuleHandleA(nullptr));
#else
    CefMainArgs args(argc, argv);
#endif

    CefRefPtr<Application> app(new Application);
    int code = CefExecuteProcess(args, app, nullptr);
    if (code >= 0)
        exit(code);

    CefSettings settings;
    settings.no_sandbox = true;
    settings.windowless_rendering_enabled = true;

    CefInitialize(args, settings, app, nullptr);
    return app;
}

CefRefPtr<CefBrowser> createBrowser(const CefRefPtr<CefClient>& client)
{
    CefWindowInfo windowInfo;
    windowInfo.SetAsWindowless(0);
    CefBrowserSettings settings;
    settings.windowless_frame_rate = 60;
    return CefBrowserHost::CreateBrowserSync(windowInfo, client, "https://www.google.com", settings, nullptr, nullptr);
}

void UI::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();
    browserId = browser->GetIdentifier();
}

}   // namespace df::ui