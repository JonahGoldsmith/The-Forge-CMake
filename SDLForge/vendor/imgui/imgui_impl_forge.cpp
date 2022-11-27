//
// Created by gamer on 11/27/2022.
//

#include "imgui_impl_forge.h"

#include "Graphics/Interfaces/IGraphics.h"

struct ImGui_ImplForge_RenderBuffers
{
    Buffer* pIndexBuffer;
    Buffer* pVertexBuffer;
    int indexBufferSize;
    int vertexBufferSize;
};

struct ImGui_ImplForge_Data
{
    Renderer* pRenderer = NULL;
    RootSignature* pRootSignature = NULL;
    Pipeline* pPipeline = NULL;
    Shader* pShader = NULL;
    Sampler* pSampler = NULL;
    Texture* pFontTexture = NULL;
    DescriptorSet* pDescriptorSetFontTexture = { NULL };

};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplForge_Data* ImGui_ImplDX12_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplForge_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

bool ImGui_ImplForge_Init(Renderer* pRenderer)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    //ImGui_ImplDX12_Data* bd = IM_NEW(ImGui_ImplDX12_Data)();
    io.BackendRendererUserData = (void*)bd;

    return true;

}