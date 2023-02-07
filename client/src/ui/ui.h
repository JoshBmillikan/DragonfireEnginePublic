//
// Created by josh on 2/6/23.
//

#pragma once

#include "widget.h"

typedef struct FT_LibraryRec_* FT_Library;

namespace df::ui {

class UIContext {
public:
    UIContext(std::unique_ptr<Widget>&& rootWidget);
    UIContext(Widget* rootWidget = nullptr) : UIContext(std::unique_ptr<Widget>(rootWidget)) {}
    ~UIContext();
    DF_NO_MOVE_COPY(UIContext);

private:
    std::unique_ptr<Widget> root;
    FT_Library freetype = nullptr;
};

}   // namespace df::ui