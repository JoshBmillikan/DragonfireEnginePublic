//
// Created by josh on 2/6/23.
//

#include "ui.h"
#include <imgui.h>

namespace df {
void renderUI(double delta)
{
    ImGui::Begin("Test");
    ImGui::Text("Frame time: %.3f (%.1f FPS)", delta, ImGui::GetIO().Framerate);
    ImGui::End();
}
}   // namespace df