//
// Created by josh on 12/30/22.
//

#pragma once
#include "application.h"

namespace df::ui {
    CefRefPtr<Application> initCEF(int argc, char** argv);
}   // namespace df::ui