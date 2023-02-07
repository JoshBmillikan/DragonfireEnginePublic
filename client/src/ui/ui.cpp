//
// Created by josh on 2/6/23.
//

#include "ui.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace df::ui {

UIContext::UIContext(std::unique_ptr<Widget>&& rootWidget) : root(std::move(rootWidget))
{
    int error = FT_Init_FreeType(&freetype);
    if (error)
        crash("Failed to load FreeType: {}", FT_Error_String(error));

    int major, minor, patch;
    FT_Library_Version(freetype, &major, &minor, &patch);
    spdlog::info("FreeType version {}.{}.{} loaded", major, minor, patch);
}

UIContext::~UIContext()
{
    FT_Done_FreeType(freetype);
}

}   // namespace df::ui