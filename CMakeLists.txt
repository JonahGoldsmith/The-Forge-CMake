cmake_minimum_required(VERSION 3.15)

project(The-Forge)


option( THE_FORGE_VULKAN                "Build Vulkan"                             ON)
option( THE_FORGE_D3D12                 "Build D3D12"                              OFF)
option( THE_FORGE_D3D11                 "Build D3D11"                              OFF)
option( THE_FORGE_METAL                 "Build Metal"                              OFF)
option( THE_FORGE_BUILD_EXAMPLES        "Build examples"                           OFF)
option( THE_FORGE_SCRIPTING_LUA         "Build Lua"                                OFF)
option( THE_FORGE_INPUT_SYSTEM                "Build Input"                              OFF)
set(THE_FORGE_EXTERNAL_CONFIG_FILE          "" CACHE STRING "Location of The-Forge config file"       )

if(NOT THE_FORGE_DIR)
	set(THE_FORGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/The-Forge" CACHE STRING "Location of bx.")
elseif(NOT IS_ABSOLUTE "${THE_FORGE_DIR}")
	get_filename_component(BX_DIR "${THE_FORGE_DIR}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()



# Make our APIs into options
option(DX12 "DirectX12 (Windows only)" OFF)
option(DX11 "DirectX11 (Windows only)" OFF)
option(EXAMPLES "The Forge examples" OFF)
option(VULKAN "Vulkan" OFF)
option(DYNAMIC_LIB "Dynamic Library" OFF)

set(ASSIMP OFF)
set(OZZ OFF)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message("Apple platform detected. Generating macOS and iOS targets.")
    set(APPLE_PLATFORM ON)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message("Linux detected. Generating Linux targets.")
    set(LINUX ON)
    set(VULKAN ON)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message("Windows detected. Generating Windows targets.")
    set(WINDOWS ON)
endif()


# Setup some sane API defaults.
set(API_SELECTED ON)

if(${VULKAN} MATCHES OFF)
    if(${DX11} MATCHES OFF)
        if(${DX12} MATCHES OFF)
            if(${METAL} MATCHES OFF)
                set(API_SELECTED OFF)
            endif()
        endif()
    endif()
endif()

# Apple platforms should always default to Metal.
if (${APPLE_PLATFORM} MATCHES ON)
    if(${METAL} MATCHES OFF)
        set(METAL ON)
    endif()

    if(${VULKAN} MATCHES ON)
        set(VULKAN OFF)
    endif()

    if(${DX11} MATCHES ON)
        set(DX11 OFF)
    endif()

    if(${DX12} MATCHES ON)
        set(DX12 OFF)
    endif()

    set(API_SELECTED ON)
endif()

if (${API_SELECTED} MATCHES OFF)
    if(${APPLE_PLATFORM} MATCHES ON)
        set(METAL ON)
    endif()

    if(${LINUX} MATCHES ON)
        set(VULKAN ON)
    endif()

    if(${WINDOWS} MATCHES ON)
        set(DX12 ON)
        # Needed due to how the profiler in forge force-links to vulkan as well.
        set(VULKAN ON)
    endif()
endif()

if(${DX12} MATCHES ON)
    set(DX11 ON)
endif()

message("The following options have been set to on:\n")
if(${APPLE_PLATFORM} MATCHES ON)
    message("Apple platform")
endif()

if(${LINUX} MATCHES ON)
    message("Linux platform")
endif()

if(${WINDOWS} MATCHES ON)
    message("Windows platform")
endif()

if(${METAL} MATCHES ON)
    message("Metal reendering API")
endif()

if(${VULKAN} MATCHES ON)
    message("Vulkan rendering API")
endif()

if(${DX11} MATCHES ON)
    message("DirectX 11 rendering API")
endif()

if(${DX12} MATCHES ON)
    message("DirectX 12 rendering API")
endif()

if(${DYNAMIC_LIB} MATCHES ON)
    message("Dynamic library")
endif()

message("\n")

# add_subdirectory(Common)
# add_subdirectory(Testing)
# add_subdirectory(Triangle)
# add_subdirectory(Quad)
# add_subdirectory(Cube)


include( ${CMAKE_CURRENT_SOURCE_DIR}/cmake/The-Forge.cmake )