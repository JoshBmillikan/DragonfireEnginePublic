//
// Created by josh on 2/6/23.
//

#include "ui.h"
#include <imgui.h>

namespace df::ui {
void init()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
}

void renderUI(double delta)
{
    ImGui::Begin("Info");
    static ULong counter = 0;
    ImGui::Text("Frame %lu", counter++);
    ImGui::Text("Frame time: %.1fms (%.1f FPS)", delta * 1000, ImGui::GetIO().Framerate);
    ImGui::End();
    ImGui::Render();
}
}   // namespace df