#  Graphics

file(GLOB THE_FORGE_SOURCES
    ${THE_FORGE_DIR}/Common_3/Graphics/*.cpp
    ${THE_FORGE_DIR}/Application/*.cpp
    ${THE_FORGE_DIR}/Application/Profiler/*.cpp
    ${THE_FORGE_DIR}/Application/Fonts/*.cpp
    ${THE_FORGE_DIR}/Application/UI/*.cpp
    # Common_3
    ${THE_FORGE_DIR}/Common_3/OS/WindowSystem/*.cpp
    ${THE_FORGE_DIR}/Common_3/OS/*.cpp
    )

if(THE_FORGE_SCRIPTING_LUA)
    file(GLOB THE_FORGE_LUA_SCRIPTING_SOURCES
        "${THE_FORGE_DIR}/Game/Scripting/*.cpp"
        )
    list(APPEND THE_FORGE_SOURCES ${THE_FORGE_LUA_SCRIPTING_SOURCES})
endif()


IF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    file(GLOB THE_FORGE_OS_DARWIN_SOURCES
        ${THE_FORGE_DIR}/Common_3/OS/Linux/*.cpp)
    list(APPEND THE_FORGE_SOURCES ${THE_FORGE_OS_DARWIN_SOURCES})
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    file(GLOB THE_FORGE_OS_WINDOWS_SOURCES
        ${THE_FORGE_DIR}/Common_3/OS/Windows/*.cpp)
    list(APPEND THE_FORGE_SOURCES ${THE_FORGE_OS_WINDOWS_SOURCES})
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    file(GLOB THE_FORGE_OS_LINUX_SOURCES
        ${THE_FORGE_DIR}/Common_3/OS/Windows/*.cpp)
    list(APPEND THE_FORGE_SOURCES ${THE_FORGE_OS_LINUX_SOURCES})
endif()

if(THE_FORGE_INPUT_SYSTEM)     
    list(APPEND THE_FORGE_SOURCES 
        "${THE_FORGE_DIR}/Common_3/Application/InputSystem.cpp"
        ${THE_FORGE_GA_INPUT_SOURCES})
endif()

list(APPEND THE_FORGE_SOURCES 
    ${THE_FORGE_SOURCES})

if(THE_FORGE_METAL)
    file(GLOB GRAPHICS_METAL_SRC
        "${THE_FORGE_DIR}/Common_3/Graphics/Metal/*.mm")
    list(APPEND THE_FORGE_SOURCES ${GRAPHICS_METAL_SRC})
endif()
if(THE_FORGE_VULKAN)
    file(GLOB GRAPHICS_VULKAN_SRC
        "${THE_FORGE_DIR}/Common_3/Graphics/Vulkan/*.cpp")    
    list(APPEND THE_FORGE_SOURCES ${GRAPHICS_VULKAN_SRC})
endif()
if(THE_FORGE_D3D12)
    file(GLOB GRAPHICS_D3D12_SRC
        "${THE_FORGE_DIR}/Common_3/Graphics/Direct3D12/*.cpp")
    list(APPEND THE_FORGE_SOURCES ${GRAPHICS_D3D12_SRC})
endif()
if(THE_FORGE_D3D11)
    file(GLOB GRAPHICS_D3D11_SRC
        "${THE_FORGE_DIR}/Common_3/Graphics/Direct3D11/*.cpp")
    list(APPEND THE_FORGE_SOURCES ${GRAPHICS_D3D11_SRC})
endif()
if(THE_FORGE_OPENGLES)
    file(GLOB GRAPHICS_OPENGLES_SRC
        "${THE_FORGE_DIR}/Common_3/Graphics/OpenGLES/*.cpp")
    list(APPEND THE_FORGE_SOURCES ${GRAPHICS_OPENGLES_SRC})
endif()

add_library(TheForge STATIC ${THE_FORGE_SOURCES})
target_include_directories(TheForge PUBLIC ${THE_FORGE_DIR})
target_link_directories(TheForge PUBLIC cpu_features)

if(THE_FORGE_SCRIPTING_LUA)
    include( ${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty/lua.cmake)
    target_link_libraries( TheForge PUBLIC lua )
endif()

if(THE_FORGE_INPUT_SYSTEM)     
    include( ${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty/ga.cmake)
    target_link_libraries( TheForge PUBLIC ga )
endif()

if(THE_FORGE_VULKAN)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty/SPIRV_Cross.cmake)
    target_link_libraries( TheForge PUBLIC SPIRV_Cross )
endif()

IF(THE_FORGE_D3D12 OR THE_FORGE_D3D11)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/3rdParty/WinPixEventRuntime.cmake)
    target_link_libraries( TheForge PRIVATE WinPixEventRuntime )
ENDIF()
