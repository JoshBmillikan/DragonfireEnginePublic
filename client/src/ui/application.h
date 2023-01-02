//
// Created by josh on 12/31/22.
//

#pragma once
#include <cef_app.h>

namespace df::ui {

class Application : public CefApp, public CefBrowserProcessHandler {
    IMPLEMENT_REFCOUNTING(Application);
public:

};

}   // namespace df::ui