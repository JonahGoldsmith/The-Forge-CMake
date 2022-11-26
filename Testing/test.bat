SET mypath=%~dp0
@echo %mypath%



%mypath%/../The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders %mypath%/../The-Forge/Common_3/Application/UI/Shaders/FSL/UI_ShaderList.fsl --compile --incremental -l DIRECT3D12
%mypath%/../The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders %mypath%/../The-Forge/Common_3/Application/UI/Shaders/FSL/UI_ShaderList.fsl --compile --incremental -l DIRECT3D11
%mypath%/../The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders %mypath%/../The-Forge/Common_3/Application/UI/Shaders/FSL/UI_ShaderList.fsl --compile --incremental -l VULKAN


%mypath%/../The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders %mypath%/../The-Forge/Common_3/Application/Fonts/Shaders/FSL/Fonts_ShaderList.fsl --compile --incremental -l DIRECT3D12
%mypath%/../The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders %mypath%/../The-Forge/Common_3/Application/Fonts/Shaders/FSL/Fonts_ShaderList.fsl --compile --incremental -l DIRECT3D11
%mypath%/../The-Forge/Common_3/Tools/ForgeShadingLanguage/fsl.py -d Shaders -b CompiledShaders %mypath%/../The-Forge/Common_3/Application/Fonts/Shaders/FSL/Fonts_ShaderList.fsl --compile --incremental -l VULKAN

