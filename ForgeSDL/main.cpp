//
//#include "Application/Interfaces/IApp.h"
//#include "Utilities/Interfaces/IFileSystem.h"
//#include "Utilities/Interfaces/ILog.h"
//#include "Utilities/Interfaces/IFileSystem.h"
//#include "Utilities/Interfaces/IThread.h"
//#include "Utilities/Interfaces/ITime.h"
//#include "OS/Interfaces/IOperatingSystem.h"
//
//#include <SDL.h>
//#include <SDL_syswm.h>
//
//
//#include "Utilities/Interfaces/IMemory.h"
//
//#define SDL_ENABLE_SYSWM_WINDOWS
//
////CPU Info
//static CpuInfo gCpu;
//
//static SDL_Window* sdl_window;
//
//int main(int argc, char** argv)
//{
//    /*
//     * Initialize Base Forge Systems
//     */
//    if (!initMemAlloc("ForgeSDL")) {
//        return EXIT_FAILURE;
//    }
//
//    FileSystemInitDesc fsDesc = {};
//    fsDesc.pAppName = "ForgeSDL";
//    if (!initFileSystem(&fsDesc)) {
//        return EXIT_FAILURE;
//    }
//    fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG, RD_LOG, "");
//
//    initLog("ForgeSDL", DEFAULT_LOG_LEVEL);
//
//    if(SDL_Init(SDL_INIT_EVERYTHING) !=0 ) {
//        return -3;
//    }
//
//    initCpuInfo(&gCpu);
//
//    sdl_window = SDL_CreateWindow("ForgeSDL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
//
//#ifdef _WINDOWS
//
//    SDL_SysWMinfo wmInfo;
//    //SDL_VERSION(&wmInfo.version); /* initialize info structure with SDL version info */
//    SDL_GetWindowWMInfo(sdl_window, &wmInfo, SDL_SYSWM_CURRENT_VERSION);
//    HWND hwnd = wmInfo.info.win.window;
//
//    //gLogWindowHandle = (HWND*)&window.handle.window; // WindowsLog.c, save the address to this handle to avoid having to adding includes to WindowsLog.c to use WindowDesc*.
//#endif
//    return 0;
//
//}