
//Core
#include "Application/Interfaces/IApp.h"
#include "Utilities/Interfaces/ILog.h"
#include "Application/Interfaces/IInput.h"
#include "Utilities/Interfaces/IFileSystem.h"

//Renderer
#include "Graphics/Interfaces/IGraphics.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"

//Math
#include "Utilities/Math/MathTypes.h"
#include <SDL.h>
#include <SDL_syswm.h>

#include "Utilities/Interfaces/ITime.h"

#include <fjs/Manager.h>
#include <fjs/Counter.h>
#include <fjs/List.h>
#include <fjs/Queue.h>

#include "Utilities/Math/BStringHashMap.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "Utilities/Interfaces/IMemory.h"

DECLARE_RENDERER_FUNCTION(void, addBuffer, Renderer* pRenderer, const BufferDesc* pDesc, Buffer** pp_buffer)
DECLARE_RENDERER_FUNCTION(void, removeBuffer, Renderer* pRenderer, Buffer* pBuffer)
DECLARE_RENDERER_FUNCTION(void, mapBuffer, Renderer* pRenderer, Buffer* pBuffer, ReadRange* pRange)
DECLARE_RENDERER_FUNCTION(void, unmapBuffer, Renderer* pRenderer, Buffer* pBuffer)
DECLARE_RENDERER_FUNCTION(
        void, cmdUpdateBuffer, Cmd* pCmd, Buffer* pBuffer, uint64_t dstOffset, Buffer* pSrcBuffer, uint64_t srcOffset, uint64_t size)
DECLARE_RENDERER_FUNCTION(
        void, cmdUpdateSubresource, Cmd* pCmd, Texture* pTexture, Buffer* pSrcBuffer, const struct SubresourceDataDesc* pSubresourceDesc)
DECLARE_RENDERER_FUNCTION(
        void, cmdCopySubresource, Cmd* pCmd, Buffer* pDstBuffer, Texture* pTexture, const struct SubresourceDataDesc* pSubresourceDesc)
DECLARE_RENDERER_FUNCTION(void, addTexture, Renderer* pRenderer, const TextureDesc* pDesc, Texture** ppTexture)
DECLARE_RENDERER_FUNCTION(void, removeTexture, Renderer* pRenderer, Texture* pTexture)
DECLARE_RENDERER_FUNCTION(void, addVirtualTexture, Cmd* pCmd, const TextureDesc* pDesc, Texture** ppTexture, void* pImageData)

static CpuInfo gCpu;
static SDL_Window* sdl_window;

static int width;
static int height;

#ifdef _WINDOWS
static HWND hwnd;
// WindowsLog.c
extern "C" HWND* gLogWindowHandle;
#endif

//Frames in Flight!
const uint gImageCount = 3;
uint gFrameIndex = 0;

static bool enableVsync = true;

/*
 * All Of Our Rendering Objects
 */
Renderer* pRenderer;


Queue*   pGraphicsQueue = NULL;
Queue* pTransferQueue = NULL;
CmdPool* pCmdPools[gImageCount] = { NULL };
Cmd*     pCmds[gImageCount] = { NULL };


CmdPool* pTransferCmdPool = NULL;
Cmd* pTransferCmd = NULL;

SwapChain*    pSwapChain = NULL;
RenderTarget* pDepthBuffer = NULL;

Fence* pTransferFence = NULL;
Fence*        pRenderCompleteFences[gImageCount] = { NULL };
Semaphore*    pImageAcquiredSemaphore = NULL;
Semaphore*    pRenderCompleteSemaphores[gImageCount] = { NULL };

Shader*   pTriangleShader = NULL;

Buffer* pTriangleVertexBuffer = NULL;
Buffer* pIndexBuffer = NULL;
Buffer* pStageBuffer = NULL;
Buffer* pCameraUniformBuffers[gImageCount] = { NULL };

Pipeline* pTrianglePipeline = NULL;

RootSignature* pRootSignature = NULL;
//DescriptorSet* pDescriptorSetTexture = NULL;
DescriptorSet* pDescriptorSetUniforms = { NULL };

struct Mesh
{
    Buffer* pVertexBuffer;
    Buffer* pIndexBuffer;
};

struct InstanceData
{
    Matrix4 model;
};

//Structure for our Vertices
struct Vertex
{
    Vector3 position;
    Vector4 color;
};

Vertex* vertices = NULL;

uint32_t* indices = NULL;

struct CameraUniforms
{
    Matrix4 projview;
};

CameraUniforms cameraUniforms;

// Setup Job Manager
fjs::ManagerOptions managerOptions;

fjs::Manager* gManager;

Matrix4 view = Matrix4::identity();

static void LoadModel(char* file)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file)) {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
            };

            /*
            vertex.uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
            };
            */

            vertex.color = {1.0f, 0.0f, 1.0f, 1.0f};

            arrpush(vertices, vertex);
            if(!indices)
                arrpush(indices, 0);
            arrpush(indices, arrlenu(indices));
        }
    }
}

bool Init()
{
    /*
    * Setup Our File Paths... For this Example we dont really need any except Shaders!
    */
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "Shaders");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "CompiledShaders");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_GPU_CONFIG, "GPUCfg");

    LoadModel("cube.obj");

    /*
     * Init the renderer with no fancy requirements
     */
    RendererDesc settings;
    memset(&settings, 0, sizeof(settings));
    settings.mD3D11Supported = true;
    settings.mGLESSupported = false;
    initRenderer("SDLForge", &settings, &pRenderer);
    //check for init success
    if (!pRenderer)
        return false;


    //Only need one queue for this example...
    QueueDesc queueDesc = {};
    queueDesc.mType = QUEUE_TYPE_GRAPHICS;
    queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
    addQueue(pRenderer, &queueDesc, &pGraphicsQueue);

    queueDesc.mType = QUEUE_TYPE_TRANSFER;
    queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
    addQueue(pRenderer, &queueDesc, &pTransferQueue);

    CmdPoolDesc transferPoolDesk = {};
    transferPoolDesk.pQueue = pTransferQueue;
    addCmdPool(pRenderer, &transferPoolDesk, &pTransferCmdPool);
    CmdDesc transferCmdDesc = {};
    transferCmdDesc.pPool = pTransferCmdPool;
    addCmd(pRenderer, &transferCmdDesc, &pTransferCmd);
    addFence(pRenderer, &pTransferFence);

    for (uint32_t i = 0; i < gImageCount; ++i)
    {
        CmdPoolDesc cmdPoolDesc = {};
        cmdPoolDesc.pQueue = pGraphicsQueue;
        addCmdPool(pRenderer, &cmdPoolDesc, &pCmdPools[i]);
        CmdDesc cmdDesc = {};
        cmdDesc.pPool = pCmdPools[i];
        addCmd(pRenderer, &cmdDesc, &pCmds[i]);

        addFence(pRenderer, &pRenderCompleteFences[i]);
        addSemaphore(pRenderer, &pRenderCompleteSemaphores[i]);
    }
    addSemaphore(pRenderer, &pImageAcquiredSemaphore);

   // initResourceLoaderInterface(pRenderer);

    //Size of our vertex buffer
    uint64_t size = sizeof(Vertex) * 3;

    BufferDesc cameraUBDesc = {};
    cameraUBDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraUBDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    cameraUBDesc.mSize = sizeof(CameraUniforms);

    for (uint32_t i = 0; i < gImageCount; ++i)
    {
        addBuffer(pRenderer, &cameraUBDesc, &pCameraUniformBuffers[i]);
    }

    /*
 * Add a buffer without using the resource loader!
 */
    BufferDesc vertDesc = {};
    vertDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    vertDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    vertDesc.mSize = sizeof(Vertex)*arrlenu(vertices);
    addBuffer(pRenderer, &vertDesc, &pTriangleVertexBuffer);

    BufferDesc stageDesc = {};
    stageDesc.mDescriptors = DESCRIPTOR_TYPE_UNDEFINED;
    stageDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    stageDesc.mSize = sizeof(Vertex) * arrlenu(vertices);

    addBuffer(pRenderer, &stageDesc, &pStageBuffer);

    ReadRange readRange = { };
    readRange.mOffset = 0;
    readRange.mSize = sizeof(Vertex) * arrlenu(vertices);

    mapBuffer(pRenderer, pStageBuffer, &readRange);
    memcpy(pStageBuffer->pCpuMappedAddress, vertices, sizeof(Vertex) * arrlenu(vertices));
    unmapBuffer(pRenderer, pStageBuffer);

    resetCmdPool(pRenderer, pTransferCmdPool);
    beginCmd(pTransferCmd);
    cmdUpdateBuffer(pTransferCmd, pTriangleVertexBuffer, 0, pStageBuffer, 0, sizeof(Vertex)*arrlenu(vertices));
    endCmd(pTransferCmd);

    QueueSubmitDesc submitDesc = {};
    submitDesc.mCmdCount = 1;
    submitDesc.mSignalSemaphoreCount = 0;
    submitDesc.mWaitSemaphoreCount = 0;
    submitDesc.ppCmds = &pTransferCmd;
    submitDesc.pSignalFence = pTransferFence;
    queueSubmit(pTransferQueue, &submitDesc);
    waitForFences(pRenderer, 1, &pTransferFence);

    removeBuffer(pRenderer, pStageBuffer);

    stageDesc.mDescriptors = DESCRIPTOR_TYPE_UNDEFINED;
    stageDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    stageDesc.mSize = sizeof(uint32_t) * arrlenu(indices);
    addBuffer(pRenderer, &stageDesc, &pStageBuffer);

    BufferDesc ibDesc = {};
    ibDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
    ibDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    ibDesc.mSize = sizeof(uint32_t)*arrlenu(indices);
    addBuffer(pRenderer, &ibDesc, &pIndexBuffer);

    readRange.mOffset = 0;
    readRange.mSize = sizeof(uint32_t) * arrlenu(indices);

    mapBuffer(pRenderer, pStageBuffer, &readRange);
    memcpy(pStageBuffer->pCpuMappedAddress, indices, sizeof(uint32_t) * arrlenu(indices ));
    unmapBuffer(pRenderer, pStageBuffer);

    resetCmdPool(pRenderer, pTransferCmdPool);
    beginCmd(pTransferCmd);
    cmdUpdateBuffer(pTransferCmd, pIndexBuffer, 0, pStageBuffer, 0, sizeof(uint32_t)*arrlenu(indices));
    endCmd(pTransferCmd);

    submitDesc.mCmdCount = 1;
    submitDesc.mSignalSemaphoreCount = 0;
    submitDesc.mWaitSemaphoreCount = 0;
    submitDesc.ppCmds = &pTransferCmd;
    submitDesc.pSignalFence = pTransferFence;
    queueSubmit(pTransferQueue, &submitDesc);
    waitForFences(pRenderer, 1, &pTransferFence);

    removeBuffer(pRenderer, pStageBuffer);

    //Set the frame index to 0
    gFrameIndex = 0;

    return true;
}

bool createSwapChain()
{

#ifdef _WINDOWS
    WindowHandle handle;
    handle.window = hwnd;
#endif

    SwapChainDesc swapChainDesc = {};
    swapChainDesc.mWindowHandle = handle;
    swapChainDesc.mPresentQueueCount = 1;
    swapChainDesc.ppPresentQueues = &pGraphicsQueue;
    swapChainDesc.mWidth = width;
    swapChainDesc.mHeight = height;
    swapChainDesc.mImageCount = gImageCount;
    swapChainDesc.mColorFormat = getRecommendedSwapchainFormat(true, true);
    swapChainDesc.mColorClearValue = { { 0.02f, 0.02f, 0.02f, 1.0f } };
    swapChainDesc.mEnableVsync = enableVsync;
    ::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

    return pSwapChain != NULL;
}

bool addDepthBuffer()
{
    // Add depth buffer
    RenderTargetDesc depthRT = {};
    depthRT.mArraySize = 1;
    depthRT.mClearValue.depth = 1.0f;
    depthRT.mClearValue.stencil = 0;
    depthRT.mDepth = 1;
    depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
    depthRT.mStartState = RESOURCE_STATE_DEPTH_WRITE;
    depthRT.mHeight = height;
    depthRT.mSampleCount = SAMPLE_COUNT_1;
    depthRT.mSampleQuality = 0;
    depthRT.mWidth = width;
    depthRT.mFlags = TEXTURE_CREATION_FLAG_ON_TILE;
    addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);

    return pDepthBuffer != NULL;
}

bool Load()
{
    ShaderLoadDesc basicShader = {};
    basicShader.mStages[0] = { "triangle.vert", NULL, 0 , NULL};
    basicShader.mStages[1] = { "triangle.frag", NULL, 0, NULL };

    addShader(pRenderer, &basicShader, &pTriangleShader);

    RootSignatureDesc rootDesc = { &pTriangleShader, 1 };
    rootDesc.mStaticSamplerCount = 0;
    addRootSignature(pRenderer, &rootDesc, &pRootSignature);
    {
        DescriptorSetDesc desc = {pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, gImageCount};
        addDescriptorSet(pRenderer, &desc, &pDescriptorSetUniforms);
    }
    for (uint32_t i = 0; i < gImageCount; ++i)
    {
        DescriptorData params[1] = {};
        params[0].pName = "cameraUniform";
        params[0].ppBuffers = &pCameraUniformBuffers[i];
        updateDescriptorSet(pRenderer, i, pDescriptorSetUniforms, 1, params);
    }

    if (!createSwapChain())
        return false;

    if(!addDepthBuffer())
        return false;

    //layout and pipeline for sphere draw
    VertexLayout vertexLayout = {};
    vertexLayout.mAttribCount = 2;
    vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
    vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
    vertexLayout.mAttribs[0].mBinding = 0;
    vertexLayout.mAttribs[0].mLocation = 0;
    vertexLayout.mAttribs[0].mOffset = 0;

    vertexLayout.mAttribs[1].mSemantic = SEMANTIC_COLOR;
    vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
    vertexLayout.mAttribs[1].mBinding = 0;
    vertexLayout.mAttribs[1].mLocation = 1;
    vertexLayout.mAttribs[1].mOffset = offsetof(Vertex, color);

//    vertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD0;
//    vertexLayout.mAttribs[2].mFormat = TinyImageFormat_R32G32_SFLOAT;
//    vertexLayout.mAttribs[2].mBinding = 0;
//    vertexLayout.mAttribs[2].mLocation = 2;
//    vertexLayout.mAttribs[2].mOffset = offsetof(Vertex, uv);

    RasterizerStateDesc sphereRasterizerStateDesc = {};
    sphereRasterizerStateDesc.mCullMode = CULL_MODE_FRONT;

    DepthStateDesc depthStateDesc = {};
    depthStateDesc.mDepthTest = true;
    depthStateDesc.mDepthWrite = true;
    depthStateDesc.mDepthFunc = CMP_LEQUAL;

    PipelineDesc desc = {};
    desc.mType = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
    pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
    pipelineSettings.mRenderTargetCount = 1;
    pipelineSettings.pDepthState = &depthStateDesc;
    pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
    pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
    pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
    pipelineSettings.mDepthStencilFormat = pDepthBuffer->mFormat;
    pipelineSettings.pRootSignature = pRootSignature;
    pipelineSettings.pShaderProgram = pTriangleShader;
    pipelineSettings.pVertexLayout = &vertexLayout;
    pipelineSettings.pRasterizerState = &sphereRasterizerStateDesc;
    pipelineSettings.mVRFoveatedRendering = true;
    addPipeline(pRenderer, &desc, &pTrianglePipeline);
    return true;
}

void Unload()
{
    waitQueueIdle(pGraphicsQueue);

    removePipeline(pRenderer, pTrianglePipeline);

    removeRenderTarget(pRenderer, pDepthBuffer);

    removeSwapChain(pRenderer, pSwapChain);

    removeDescriptorSet(pRenderer, pDescriptorSetUniforms);

    removeRootSignature(pRenderer, pRootSignature);

    removeShader(pRenderer, pTriangleShader);

}

void Resize(int inWidth, int inHeight)
{
    width = inWidth; height = inHeight;
    Unload();
    Load();
}

void Update(float delta)
{
    const float aspectInverse = (float)height / (float)width;

    Matrix4 proj = Matrix4::perspective((45.0f*3.14)/180.0f, aspectInverse, 0.1f, 100.0f);

    // Matrix4 proj = Matrix4::orthographic(0, mSettings.mWidth, 0, mSettings.mHeight, 0.1f, 100.0f);

    //view = Matrix4::scale(Vector3(0.5));
    view.setTranslation(Vector3(0.0, 0.0, 10));
    view *= Matrix4::rotation((15.0f*3.14)/180.0f * delta, vec3(0.0, 1.0, 1.0));
    //model.setTranslation(Vector3(0, 0, 10));

    //model *= Matrix4::rotation(45.0f, Vector3(0, 0, 1));
    cameraUniforms.projview = proj * view;
}

void Draw()
{

    if (pSwapChain->mEnableVsync != enableVsync)
    {
        waitQueueIdle(pGraphicsQueue);
        ::toggleVSync(pRenderer, &pSwapChain);
    }

    uint32_t swapchainImageIndex;
    acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL, &swapchainImageIndex);

    RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];
    Semaphore*    pRenderCompleteSemaphore = pRenderCompleteSemaphores[gFrameIndex];
    Fence*        pRenderCompleteFence = pRenderCompleteFences[gFrameIndex];

    // Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
    FenceStatus fenceStatus;
    getFenceStatus(pRenderer, pRenderCompleteFence, &fenceStatus);
    if (fenceStatus == FENCE_STATUS_INCOMPLETE)
        waitForFences(pRenderer, 1, &pRenderCompleteFence);

    ReadRange range = {};
    range.mSize = sizeof(CameraUniforms);
    range.mOffset = 0;

    // Update uniform buffers
    mapBuffer(pRenderer, pCameraUniformBuffers[gFrameIndex], &range);
    memcpy(pCameraUniformBuffers[gFrameIndex]->pCpuMappedAddress, &cameraUniforms, sizeof(CameraUniforms));
    unmapBuffer(pRenderer, pCameraUniformBuffers[gFrameIndex]);

//    BufferUpdateDesc viewProjCbv = { pProjViewUniformBuffer[gFrameIndex] };
//    beginUpdateResource(&viewProjCbv);
//    *(UniformBlock*)viewProjCbv.pMappedData = uniformBlock;
//    endUpdateResource(&viewProjCbv, NULL);

    // Reset cmd pool for this frame
    resetCmdPool(pRenderer, pCmdPools[gFrameIndex]);

    Cmd* cmd = pCmds[gFrameIndex];
    beginCmd(cmd);

    RenderTargetBarrier barriers[] = {
            { pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET },
    };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

    // simply record the screen cleaning command
    LoadActionsDesc loadActions = {};
    loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
    loadActions.mLoadActionDepth = LOAD_ACTION_CLEAR;
    loadActions.mClearDepth.depth = 1.0f;
    cmdBindRenderTargets(cmd, 1, &pRenderTarget, pDepthBuffer, &loadActions, NULL, NULL, -1, -1);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

    const uint32_t sphereVbStride = sizeof(Vertex);

    Pipeline* pipeline = pTrianglePipeline;

    cmdBindPipeline(cmd, pipeline);
    cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetUniforms);
    cmdBindVertexBuffer(cmd, 1, &pTriangleVertexBuffer, &sphereVbStride, NULL);
    cmdBindIndexBuffer(cmd, pIndexBuffer, INDEX_TYPE_UINT32, 0);

    cmdDrawIndexed(cmd, arrlenu(indices), 0, 0);

    loadActions = {};
    loadActions.mLoadActionsColor[0] = LOAD_ACTION_LOAD;
    cmdBindRenderTargets(cmd, 1, &pRenderTarget, nullptr, &loadActions, NULL, NULL, -1, -1);

    cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);

    barriers[0] = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

    endCmd(cmd);

    QueueSubmitDesc submitDesc = {};
    submitDesc.mCmdCount = 1;
    submitDesc.mSignalSemaphoreCount = 1;
    submitDesc.mWaitSemaphoreCount = 1;
    submitDesc.ppCmds = &cmd;
    submitDesc.ppSignalSemaphores = &pRenderCompleteSemaphore;
    submitDesc.ppWaitSemaphores = &pImageAcquiredSemaphore;
    submitDesc.pSignalFence = pRenderCompleteFence;
    queueSubmit(pGraphicsQueue, &submitDesc);
    QueuePresentDesc presentDesc = {};
    presentDesc.mIndex = swapchainImageIndex;
    presentDesc.mWaitSemaphoreCount = 1;
    presentDesc.pSwapChain = pSwapChain;
    presentDesc.ppWaitSemaphores = &pRenderCompleteSemaphore;
    presentDesc.mSubmitDone = true;

    queuePresent(pGraphicsQueue, &presentDesc);

    gFrameIndex = (gFrameIndex + 1) % gImageCount;
}

void Exit()
{
    for (uint32_t i = 0; i < gImageCount; ++i)
    {
        removeBuffer(pRenderer, pCameraUniformBuffers[i]);
    }

    removeBuffer(pRenderer, pIndexBuffer);
    removeBuffer(pRenderer, pTriangleVertexBuffer);

    removeFence(pRenderer, pTransferFence);
    removeCmd(pRenderer, pTransferCmd);
    removeCmdPool(pRenderer, pTransferCmdPool);

    for (uint32_t i = 0; i < gImageCount; ++i)
    {
        removeFence(pRenderer, pRenderCompleteFences[i]);
        removeSemaphore(pRenderer, pRenderCompleteSemaphores[i]);

        removeCmd(pRenderer, pCmds[i]);
        removeCmdPool(pRenderer, pCmdPools[i]);
    }
    removeSemaphore(pRenderer, pImageAcquiredSemaphore);

    //exitResourceLoaderInterface(pRenderer);

    removeQueue(pRenderer, pTransferQueue);

    removeQueue(pRenderer, pGraphicsQueue);

    arrfree(vertices);
    arrfree(indices);

    exitRenderer(pRenderer);
    pRenderer = NULL;
}

//------------------------------------------------------------------------
// STATIC HELPER FUNCTIONS
//------------------------------------------------------------------------

static inline float CounterToSecondsElapsed(int64_t start, int64_t end)
{
    return (float)(end - start) / (float)1e6;
}

/*
 * MAIN FUNCTION...
 * INITIALIZATION
 * WINDOWING
 * EVENTS
 * MEMORY
 * FILESYSTEM
 * LOGGING
 */

void engineTick(int* x)
{
    bool quit = false;
    int64_t lastCounter = getUSec(false);


    while(!quit)
    {
        int64_t counter = getUSec(false);
        float   deltaTime = CounterToSecondsElapsed(lastCounter, counter);
        lastCounter = counter;

        // if framerate appears to drop below about 6, assume we're at a breakpoint and simulate 20fps.
        if (deltaTime > 0.15f)
            deltaTime = 0.05f;

        SDL_Event e;
        if(SDL_PollEvent(&e) > 0)
        {
            switch(e.type)
            {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    switch(e.window.event)
                    {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            LOGF(LogLevel::eINFO, "Window Size Updating... %d, %d", e.window.data1, e.window.data2);
                            Resize(e.window.data1, e.window.data2);
                            break;
                    }
            }
        }

        Update(deltaTime);

        Draw();

    }
    Unload();
    Exit();

    exitLog();

    exitFileSystem();

    exitMemAlloc();
}

void main_func(fjs::Manager* manager)
{
    int count = 1;
    fjs::Counter counter(manager);

    fjs::JobInfo test_job(&counter, engineTick, &count);

    manager->ScheduleJob(fjs::JobPriority::Normal, test_job);
    manager->WaitForCounter(&counter);
}

int main(int argc, char** argv)
{

    if (!initMemAlloc("SDLForge"))
        return EXIT_FAILURE;

    FileSystemInitDesc fsDesc = {};
    fsDesc.pAppName = "SDLForge";
    if (!initFileSystem(&fsDesc)) {
        return EXIT_FAILURE;

    }
    fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "");

#ifdef ENABLE_MTUNER
    rmemInit(0);
#endif

    initLog("SDLForge", DEFAULT_LOG_LEVEL);

#ifdef ENABLE_FORGE_STACKTRACE_DUMP
    if (!WindowsStackTrace::Init())
		return EXIT_FAILURE;
#endif

    //Used for automated testing, if enabled app will exit after 120 frames
#if defined(AUTOMATED_TESTING)
    uint32_t frameCounter = 0;
	uint32_t targetFrameCount = 120;
#endif

    initCpuInfo(&gCpu);

    if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        return -3;
    }

    sdl_window = SDL_CreateWindow("ForgeSDL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    width = 1280;
    height = 720;

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(sdl_window, &wmInfo);
    hwnd = wmInfo.info.win.window;

#ifdef _WINDOWS
    gLogWindowHandle = (HWND*)hwnd; // WindowsLog.c, save the address to this handle to avoid having to adding includes to WindowsLog.c to use WindowDesc*.
#endif
    Timer t;
    initTimer(&t);

    if(!Init())
    {
        return -2;
    }

    if(!Load())
    {
        return -5;
    }

    LOGF(LogLevel::eINFO, "Application Init+Load+Reload %fms", getTimerMSec(&t, false) / 1000.0f);

    managerOptions.NumFibers = managerOptions.NumThreads * 10;
    managerOptions.ThreadAffinity = true;

    managerOptions.HighPriorityQueueSize = 128;
    managerOptions.NormalPriorityQueueSize = 256;
    managerOptions.LowPriorityQueueSize = 256;

    managerOptions.ShutdownAfterMainCallback = true;

// Manager
    fjs::Manager manager(managerOptions);

    gManager = &manager;

    manager.Run(main_func);

    return 0;
}


/*
 * These are required for some reason when turning off Forge Base OS Files?
 */
void requestShutdown()
{
    PostQuitMessage(0);
}

void requestReset(const ResetDesc* pResetDesc)
{

}

void requestReload(const ReloadDesc* pReloadDesc)
{

}