
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
#include "vendor/imgui/imgui_impl_sdl.h"

#include "Utilities/Interfaces/IMemory.h"



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
CmdPool* pCmdPools[gImageCount] = { NULL };
Cmd*     pCmds[gImageCount] = { NULL };

SwapChain*    pSwapChain = NULL;

Fence*        pRenderCompleteFences[gImageCount] = { NULL };
Semaphore*    pImageAcquiredSemaphore = NULL;
Semaphore*    pRenderCompleteSemaphores[gImageCount] = { NULL };

Shader*   pTriangleShader = NULL;

Buffer* pTriangleVertexBuffer = NULL;

Pipeline* pTrianglePipeline = NULL;

RootSignature* pRootSignature = NULL;

//Structure for our Vertices
struct Vertex
{
    Vector3 position;
    Vector4 color;
};

//Basic Vertices for a Triangle
Vertex vertices[3] = {
        {{0.0, 0.5, 0.0}, {1.0, 0.0, 0.0, 1.0}},
        {{0.5, -0.5, 0.0}, {0.0, 1.0, 0.0, 1.0}},
        {{-0.5, -0.5, 0.0}, {0.0, 0.0, 1.0, 1.0}}
};

bool Init()
{
    /*
    * Setup Our File Paths... For this Example we dont really need any except Shaders!
    */
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "Shaders");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_BINARIES, "CompiledShaders");
    fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_GPU_CONFIG, "GPUCfg");

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

    initResourceLoaderInterface(pRenderer);

    //Size of our vertex buffer
    uint64_t size = sizeof(Vertex) * 3;

    //Use the Resource Loader to Create our Vertex Buffer
    //Notice How We are using GPU Memory Usage flag...
    //Cannot access this from the CPU after creation
    BufferLoadDesc loadDesc = {};
    loadDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    loadDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    loadDesc.mDesc.mSize = sizeof(vertices);
    loadDesc.pData = vertices;
    loadDesc.ppBuffer = &pTriangleVertexBuffer;
    addResource(&loadDesc, NULL);

    /*
     * Waits on the resource threads...
     */
    waitForAllResourceLoads();

    // setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.Fonts->AddFontFromFileTTF("verdana.ttf", 18.0f, NULL, NULL);

    // Init IMGUI for SDL2....
    ImGui_ImplSDL2_InitForD3D(sdl_window);

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

bool Load()
{
    ShaderLoadDesc basicShader = {};
    basicShader.mStages[0] = { "triangle.vert", NULL, 0 , NULL};
    basicShader.mStages[1] = { "triangle.frag", NULL, 0, NULL };

    addShader(pRenderer, &basicShader, &pTriangleShader);

    RootSignatureDesc rootDesc = { &pTriangleShader, 1 };
    rootDesc.mStaticSamplerCount = 0;
    addRootSignature(pRenderer, &rootDesc, &pRootSignature);

    if (!createSwapChain())
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
    vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
    vertexLayout.mAttribs[1].mBinding = 0;
    vertexLayout.mAttribs[1].mLocation = 1;
    vertexLayout.mAttribs[1].mOffset = offsetof(Vertex, color);

    RasterizerStateDesc rasterizerStateDesc = {};
    rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

    DepthStateDesc depthStateDesc = {};
    depthStateDesc.mDepthTest = false;
    depthStateDesc.mDepthWrite = false;

    BlendStateDesc blendStateDesc = {};
    blendStateDesc.mSrcAlphaFactors[0] = BC_SRC_ALPHA;
    blendStateDesc.mDstAlphaFactors[0] = BC_ONE_MINUS_SRC_ALPHA;
    blendStateDesc.mSrcFactors[0] = BC_SRC_ALPHA;
    blendStateDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;
    blendStateDesc.mMasks[0] = ALL;
    blendStateDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
    blendStateDesc.mIndependentBlend = false;

    // VertexLayout for sprite drawing.
    PipelineDesc desc = {};
    desc.mType = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
    pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
    pipelineSettings.mRenderTargetCount = 1;
    pipelineSettings.pDepthState = &depthStateDesc;
    pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
    pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
    pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
    pipelineSettings.mDepthStencilFormat = TinyImageFormat_UNDEFINED;
    pipelineSettings.pRootSignature = pRootSignature;
    pipelineSettings.pShaderProgram = pTriangleShader;
    pipelineSettings.pRasterizerState = &rasterizerStateDesc;
    pipelineSettings.pBlendState = &blendStateDesc;
    pipelineSettings.pVertexLayout = &vertexLayout;
    addPipeline(pRenderer, &desc, &pTrianglePipeline);

    return true;
}

void Unload()
{
    waitQueueIdle(pGraphicsQueue);


    removePipeline(pRenderer, pTrianglePipeline);

    removeSwapChain(pRenderer, pSwapChain);

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

    // Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
    Fence*      pNextFence = pRenderCompleteFences[gFrameIndex];
    FenceStatus fenceStatus;
    getFenceStatus(pRenderer, pNextFence, &fenceStatus);
    if (fenceStatus == FENCE_STATUS_INCOMPLETE)
    {
        waitForFences(pRenderer, 1, &pNextFence);
    }

    resetCmdPool(pRenderer, pCmdPools[gFrameIndex]);

    RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];

    Semaphore* pRenderCompleteSemaphore = pRenderCompleteSemaphores[gFrameIndex];
    Fence*     pRenderCompleteFence = pRenderCompleteFences[gFrameIndex];

    // simply record the screen cleaning command
    LoadActionsDesc loadActions = {};
    loadActions.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
    loadActions.mClearColorValues[0] = pRenderTarget->mClearValue;

    Cmd* cmd = pCmds[gFrameIndex];
    beginCmd(cmd);

    RenderTargetBarrier barriers[] = {
            { pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET },
    };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

    cmdBindRenderTargets(cmd, 1, &pRenderTarget, NULL, &loadActions, NULL, NULL, -1, -1);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

    cmdBindPipeline(cmd, pTrianglePipeline);
    uint32_t vertexStride = sizeof(Vertex);
    cmdBindVertexBuffer(cmd, 1, &pTriangleVertexBuffer, &vertexStride, NULL);
    cmdDraw(cmd, 3, 0);

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
    presentDesc.mIndex = gFrameIndex;
    presentDesc.mWaitSemaphoreCount = 1;
    presentDesc.ppWaitSemaphores = &pRenderCompleteSemaphore;
    presentDesc.pSwapChain = pSwapChain;
    presentDesc.mSubmitDone = true;
    queuePresent(pGraphicsQueue, &presentDesc);

    gFrameIndex = (gFrameIndex + 1) % gImageCount;
}

void Exit()
{

    removeResource(pTriangleVertexBuffer);

    for (uint32_t i = 0; i < gImageCount; ++i)
    {
        removeFence(pRenderer, pRenderCompleteFences[i]);
        removeSemaphore(pRenderer, pRenderCompleteSemaphores[i]);

        removeCmd(pRenderer, pCmds[i]);
        removeCmdPool(pRenderer, pCmdPools[i]);
    }
    removeSemaphore(pRenderer, pImageAcquiredSemaphore);

    exitResourceLoaderInterface(pRenderer);

    removeQueue(pRenderer, pGraphicsQueue);

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