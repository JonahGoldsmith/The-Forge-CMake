cmake_minimum_required(VERSION 3.6)

project(Triangle)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC The-Forge)

target_include_directories(${PROJECT_NAME} PUBLIC The-Forge)


if (${WINDOWS} MATCHES ON)
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/OS/ThirdParty/OpenSource/winpixeventruntime/bin/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/WinPixEventRuntime.dll)

    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Graphics/ThirdParty/OpenSource/ags/ags_lib/lib/amd_ags_x64.dll
            ${CMAKE_CURRENT_BINARY_DIR}/amd_ags_x64.dll)

    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Graphics/ThirdParty/OpenSource/DirectXShaderCompiler/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/dxcompiler.dll)

    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Graphics/ThirdParty/OpenSource/DirectXShaderCompiler/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/dxil.dll)

    find_package(PythonInterp REQUIRED)

    add_custom_command(TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Application/UI/Shaders/FSL/UI_ShaderList.fsl --compile --incremental -l DIRECT3D12)
    add_custom_command(TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Application/UI/Shaders/FSL/UI_ShaderList.fsl --compile --incremental -l DIRECT3D11)
    add_custom_command(TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Application/UI/Shaders/FSL/UI_ShaderList.fsl --compile --incremental -l VULKAN)

    add_custom_command(TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Application/Fonts/Shaders/FSL/Fonts_ShaderList.fsl --compile --incremental -l DIRECT3D12)

    add_custom_command(TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Application/Fonts/Shaders/FSL/Fonts_ShaderList.fsl --compile --incremental -l DIRECT3D11)

    add_custom_command(TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Application/Fonts/Shaders/FSL/Fonts_ShaderList.fsl --compile --incremental -l VULKAN)

    add_custom_command(TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ShaderList.fsl --compile -l DIRECT3D11)

    add_custom_command(TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ShaderList.fsl --compile -l DIRECT3D12)

    add_custom_command(TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND "${PYTHON_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ShaderList.fsl --compile -l VULKAN)

endif ()

#add_custom_command(
#        TARGET Testing PRE_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory
#        ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
#        ${CMAKE_CURRENT_BINARY_DIR}/Shaders)

#add_custom_command(
#        TARGET Testing PRE_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory
#        ${CMAKE_CURRENT_SOURCE_DIR}/CompiledShaders
#        ${CMAKE_CURRENT_BINARY_DIR}/CompiledShaders)

add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/GPUCfg
        ${CMAKE_CURRENT_BINARY_DIR}/GPUCfg)
