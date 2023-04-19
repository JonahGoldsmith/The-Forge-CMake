#  Graphics

file(GLOB THE_FORGE_SOURCES
    "${THE_FORGE_DIR}/Common_3/Graphics/*.cpp"
    "${THE_FORGE_DIR}/Application/*.cpp"
    "${THE_FORGE_DIR}/Application/Profiler/*.cpp"
    "${THE_FORGE_DIR}/Application/Fonts/*.cpp"
    "${THE_FORGE_DIR}/Application/UI/*.cpp"
    )

if(THE_FORGE_SCRIPTING_LUA)
    file(GLOB THE_FORGE_LUA_SCRIPTING_SOURCES
        "${THE_FORGE_DIR}/Game/Scripting/*.cpp"
        )
    list(APPEND THE_FORGE_SRCS ${THE_FORGE_LUA_SCRIPTING_SOURCES})
endif()

if(THE_FORGE_INPUT_SYSTEM)     
    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    endif()
    list(APPEND THE_FORGE_SRCS 
        "${THE_FORGE_DIR}/Common_3/Application/InputSystem.cpp"
        ${THE_FORGE_GA_INPUT_SOURCES})
endif()

list(APPEND THE_FORGE_SRCS 
    ${THE_FORGE_SOURCES})

if(THE_FORGE_METAL)
    file(GLOB GRAPHICS_METAL_SRC
        "${THE_FORGE_DIR}/Common_3/Graphics/Metal/*.mm")
    list(APPEND THE_FORGE_SRCS ${GRAPHICS_METAL_SRC})
endif()
if(THE_FORGE_VULKAN)
    file(GLOB GRAPHICS_VULKAN_SRC
        "${THE_FORGE_DIR}/Common_3/Graphics/Vulkan/*.cpp")    
    list(APPEND THE_FORGE_SRCS ${GRAPHICS_VULKAN_SRC})
endif()
if(THE_FORGE_D3D12)
    file(GLOB GRAPHICS_D3D12_SRC
        "${THE_FORGE_DIR}/Common_3/Graphics/Direct3D12/*.cpp")
    list(APPEND THE_FORGE_SRCS ${GRAPHICS_D3D12_SRC})
endif()
if(THE_FORGE_D3D11)
    file(GLOB GRAPHICS_D3D11_SRC
        "${THE_FORGE_DIR}/Common_3/Graphics/Direct3D11/*.cpp")
    list(APPEND THE_FORGE_SRCS ${GRAPHICS_D3D11_SRC})
endif()
if(THE_FORGE_OPENGLES)
    file(GLOB GRAPHICS_OPENGLES_SRC
        "${THE_FORGE_DIR}/Common_3/Graphics/OpenGLES/*.cpp")
    list(APPEND THE_FORGE_SRCS ${GRAPHICS_OPENGLES_SRC})
endif()

add_library(TheForge STATIC ${THE_FORGE_SRCS})

if(THE_FORGE_SCRIPTING_LUA)
    include(3rdParty/lua.cmake)
    target_link_libraries( TheForge PRIVATE lua )
endif()
if(THE_FORGE_INPUT_SYSTEM)     
    include(3rdParty/ga.cmake)
    target_link_libraries( TheForge PRIVATE ga )
endif()


