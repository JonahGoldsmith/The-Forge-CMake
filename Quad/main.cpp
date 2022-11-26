/*
 * Basic Example For Rendering a Triangle using The-Forge
 */

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
#include "Utilities/Interfaces/IMemory.h"

//Frames in Flight!
const uint gImageCount = 3;
uint gFrameIndex = 0;

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

Shader*   pQuadShader = NULL;

Buffer* pQuadVertexBuffer = NULL;
Buffer* pQuadIndexBuffer = NULL;

Pipeline* pQuadPipeline = NULL;

RootSignature* pRootSignature = NULL;

//Structure for our Vertices
struct Vertex
{
    Vector3 position;
    Vector4 color;
};

//Basic Vertices for a Triangle
Vertex vertices[4] = {
        {{-0.5, 0.5, 0.0}, {1.0, 0.0, 0.0, 1.0}},
        {{0.5, -0.5, 0.0}, {0.0, 1.0, 0.0, 1.0}},
        {{-0.5, -0.5, 0.0}, {0.0, 0.0, 1.0, 1.0}},
        {{0.5, 0.5, 0.0}, {1.0, 1.0, 1.0, 1.0}}
};

uint16_t indices[] = {
        0, 1, 2, 0, 3, 1
};

class Triangle : public IApp
{

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
        initRenderer(GetName(), &settings, &pRenderer);
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
        loadDesc.ppBuffer = &pQuadVertexBuffer;
        addResource(&loadDesc, NULL);

        BufferLoadDesc ibDesc = {};
        ibDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
        ibDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        ibDesc.mDesc.mSize = sizeof(indices);
        ibDesc.pData = indices;
        ibDesc.ppBuffer = &pQuadIndexBuffer;
        addResource(&ibDesc, NULL);

        //This commented out section is for loading the Font System and User Interface!
        /*
        // Load fonts
        FontDesc font = {};
        font.pFontPath = "TitilliumText/TitilliumText-Bold.otf";
        fntDefineFonts(&font, 1, &gFontID);

        FontSystemDesc fontRenderDesc = {};
        fontRenderDesc.pRenderer = pRenderer;
        if (!initFontSystem(&fontRenderDesc))
            return false; // report?

        // Initialize Forge User Interface Rendering
        UserInterfaceDesc uiRenderDesc = {};
        uiRenderDesc.pRenderer = pRenderer;
        initUserInterface(&uiRenderDesc);

        // Initialize micro profiler and its UI.
        ProfilerDesc profiler = {};
        profiler.pRenderer = pRenderer;
        profiler.mWidthUI = mSettings.mWidth;
        profiler.mHeightUI = mSettings.mHeight;
        initProfiler(&profiler);
        */

        /*
         * Waits on the resource threads...
         */
        waitForAllResourceLoads();

        /*
         * Initialize the Input System
         */
        InputSystemDesc inputDesc = {};
        inputDesc.pRenderer = pRenderer;
        inputDesc.pWindow = pWindow;
        if (!initInputSystem(&inputDesc))
            return false;

        //Set the frame index to 0
        gFrameIndex = 0;

        return true;
    }

    void Exit()
    {
        exitInputSystem();

        removeResource(pQuadIndexBuffer);
        removeResource(pQuadVertexBuffer);

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

    bool Load(ReloadDesc* pReloadDesc)
    {
        if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
        {
            ShaderLoadDesc basicShader = {};
            basicShader.mStages[0] = { "triangle.vert", NULL, 0 , NULL};
            basicShader.mStages[1] = { "triangle.frag", NULL, 0, NULL };

            addShader(pRenderer, &basicShader, &pQuadShader);

            RootSignatureDesc rootDesc = { &pQuadShader, 1 };
            rootDesc.mStaticSamplerCount = 0;
            addRootSignature(pRenderer, &rootDesc, &pRootSignature);

        }

        if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
        {
            if (!addSwapChain())
                return false;

        }

        if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
        {
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
            pipelineSettings.pShaderProgram = pQuadShader;
            pipelineSettings.pRasterizerState = &rasterizerStateDesc;
            pipelineSettings.pBlendState = &blendStateDesc;
            pipelineSettings.pVertexLayout = &vertexLayout;
            addPipeline(pRenderer, &desc, &pQuadPipeline);
        }

        return true;

    }


    void Unload(ReloadDesc* pReloadDesc)
    {
        waitQueueIdle(pGraphicsQueue);

        if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
        {
            removePipeline(pRenderer, pQuadPipeline);
        }

        if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
        {
            removeSwapChain(pRenderer, pSwapChain);
           // removeRenderTarget(pRenderer, pDepthBuffer);
        }

        if (pReloadDesc->mType & RELOAD_TYPE_SHADER)
        {
           // removeDescriptorSets();
            removeRootSignature(pRenderer, pRootSignature);
            removeShader(pRenderer, pQuadShader);
        }
    }

    void Update(float delta)
    {
        updateInputSystem(delta, mSettings.mWidth, mSettings.mHeight);
    }

    void Draw()
    {
        if (pSwapChain->mEnableVsync != mSettings.mVSyncEnabled)
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

        cmdBindPipeline(cmd, pQuadPipeline);
        uint32_t vertexStride = sizeof(Vertex);
        cmdBindVertexBuffer(cmd, 1, &pQuadVertexBuffer, &vertexStride, NULL);
        cmdBindIndexBuffer(cmd, pQuadIndexBuffer, INDEX_TYPE_UINT16, 0);
        cmdDrawIndexed(cmd, 6, 0, 0);

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


    const char* GetName() {return "Triangle";};


    bool addSwapChain()
    {
        SwapChainDesc swapChainDesc = {};
        swapChainDesc.mWindowHandle = pWindow->handle;
        swapChainDesc.mPresentQueueCount = 1;
        swapChainDesc.ppPresentQueues = &pGraphicsQueue;
        swapChainDesc.mWidth = mSettings.mWidth;
        swapChainDesc.mHeight = mSettings.mHeight;
        swapChainDesc.mImageCount = gImageCount;
        swapChainDesc.mColorFormat = getRecommendedSwapchainFormat(true, true);
        swapChainDesc.mColorClearValue = { { 0.02f, 0.02f, 0.02f, 1.0f } };
        swapChainDesc.mEnableVsync = mSettings.mVSyncEnabled;
        ::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

        return pSwapChain != NULL;
    }

};

DEFINE_APPLICATION_MAIN(Triangle)