//
// Created by gamer on 11/27/2022.
//

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

struct Renderer;
struct Cmd;


IMGUI_IMPL_API bool     ImGui_ImplForge_Init(Renderer* pRenderer);
IMGUI_IMPL_API void     ImGui_ImplForge_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplForge_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplForge_RenderDrawData(ImDrawData* draw_data, Cmd* pCmd);
