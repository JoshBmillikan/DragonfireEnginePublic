//
// Created by josh on 12/30/22.
//

#include "ui.h"
#include "application.h"

namespace df::ui {
CefRefPtr<Application> initCEF(int argc, char** argv)
{
#ifdef _WIN32
    CefMainArgs args(GetModuleHandle(nullptr));
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
    settings.background_color = CefColorSetARGB(0x00, 0xFF, 0xFF, 0xFF);

    CefInitialize(args, settings, app, nullptr);
    return app;
}
}   // namespace df::ui