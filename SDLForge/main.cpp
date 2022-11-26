
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

#include "Utilities/Interfaces/IMemory.h"
#include "Utilities/Interfaces/ITime.h"

static CpuInfo gCpu;
static SDL_Window* sdl_window;

#ifdef _WINDOWS
// WindowsLog.c
extern "C" HWND* gLogWindowHandle;
#endif

bool Init()
{
    return true;
}

bool Load()
{
    return true;
}

void Unload()
{

}

void Resize(int width, int height)
{
    Unload();
    Load();
}

void Update(float delta)
{

}

void Draw()
{

}

void Exit()
{

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

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(sdl_window, &wmInfo);
    HWND hwnd = wmInfo.info.win.window;

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