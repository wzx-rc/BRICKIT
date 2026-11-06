@echo off
rem BrickIt: the native brick builder, built on the WZX Engine renderer (vp3d), the engine in the submodule engine\.
rem Builds the shaders, the vp3d module, Dear ImGui and BrickIt.exe into build\bin (or %VP3D_OUT%).
rem Usage: build.bat [debug]      (first: git submodule update --init)
setlocal
cd /d "%~dp0"
set "E=engine"
if not exist "%E%\src\vp3d\Renderer.cpp" (
    echo The engine submodule is missing: git submodule update --init
    exit /b 1
)

if defined VCToolsInstallDir goto :havevc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\brickit_vsdir.txt"
set /p VSDIR=<"%TEMP%\brickit_vsdir.txt"
if not exist "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" (
    echo Visual Studio C++ build tools not found.
    exit /b 1
)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
:havevc

set "CFG=/O2 /DNDEBUG /MT"
if /i "%1"=="debug" set "CFG=/Od /Zi /MTd /D_DEBUG"

set "OUT=build\bin"
if defined VP3D_OUT set "OUT=%VP3D_OUT%"
if not exist "%OUT%\shaders" mkdir "%OUT%\shaders"
for %%D in (vp3d imgui brickit res) do if not exist build\obj\%%D mkdir build\obj\%%D
if not exist build\lib mkdir build\lib
set "SH=%E%\src\vp3d\shaders"

echo [1/4] Shaders (engine)
set "DXC=%WindowsSdkVerBinPath%x64\dxc.exe"
for %%S in (PathTrace Denoise Tonemap DDGIUpdate DDGIRaster DDGIBlend GIShade GIFade PostFX RCVisibility RCTrace RCGather RCSProbes RCSTrace RCSProbeSH RCSGather SunShadowTrace SunShadowFilter ReflectTrace ReflectFilter SRCClear SRCInsert SRCAlloc SRCCount SRCTrace SRCMerge SRCIrradiance SRCFilter SRCGather ResolveTM) do (
    "%DXC%" -nologo -T cs_6_5 -E main -HV 2021 -O3 -I %SH% %SH%\%%S.hlsl -Fo "%OUT%\shaders\%%S.cso" || exit /b 1
)
"%DXC%" -nologo -T cs_6_5 -E main -HV 2021 -O3 -I %SH% %SH%\SSAO.hlsl -Fo "%OUT%\shaders\SSAO.cso" || exit /b 1
"%DXC%" -nologo -T cs_6_5 -E main -HV 2021 -O3 -DSSAO_MS -I %SH% %SH%\SSAO.hlsl -Fo "%OUT%\shaders\SSAO_MS.cso" || exit /b 1
"%DXC%" -nologo -T cs_6_5 -E main -HV 2021 -O3 -DSSAO_BLUR -I %SH% %SH%\SSAO.hlsl -Fo "%OUT%\shaders\SSAOBlur.cso" || exit /b 1
for %%V in (VSMain VSWire VSShadow VSBackground VSProbe) do (
    "%DXC%" -nologo -T vs_6_0 -E %%V -HV 2021 -O3 -DVP3D_DDGI_PROBES -I %SH% %SH%\Raster.hlsl -Fo "%OUT%\shaders\Raster_%%V.cso" || exit /b 1
)
for %%P in (PSMain PSGhost PSId PSSelect PSWire PSBackground PSProbe PSProbeBackground) do (
    "%DXC%" -nologo -T ps_6_5 -E %%P -HV 2021 -O3 -DVP3D_DDGI_PROBES -DVP3D_RT_SHADOWS -I %SH% %SH%\Raster.hlsl -Fo "%OUT%\shaders\Raster_%%P.cso" || exit /b 1
)
"%DXC%" -nologo -T vs_6_0 -E VSGrid -HV 2021 -O3 %SH%\EditorGrid.hlsl -Fo "%OUT%\shaders\EditorGrid_VSGrid.cso" || exit /b 1
"%DXC%" -nologo -T ps_6_0 -E PSGrid -HV 2021 -O3 %SH%\EditorGrid.hlsl -Fo "%OUT%\shaders\EditorGrid_PSGrid.cso" || exit /b 1
rem assets: the engine's (HDRI, themes), then BrickIt's on top
if exist %E%\assets\hdri xcopy /E /I /Y /Q %E%\assets\hdri "%OUT%\assets\hdri" >nul
if exist %E%\assets\themes xcopy /E /I /Y /Q %E%\assets\themes "%OUT%\assets\themes" >nul
xcopy /E /I /Y /Q assets "%OUT%\assets" >nul

set "CFLAGS=/nologo /std:c++17 /EHsc /W3 /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /DIMGUI_DEFINE_MATH_OPERATORS %CFG% /Isrc /I%E%\src /I%E%\third_party\imgui /Ibuild\obj"
del /q build\obj\vp3d\*.obj build\obj\brickit\*.obj 2>nul

echo [2/4] vp3d module (engine)
cl %CFLAGS% /c %E%\src\vp3d\*.cpp /Fo:build\obj\vp3d\ || exit /b 1
lib /nologo /OUT:build\lib\vp3d.lib build\obj\vp3d\*.obj || exit /b 1

echo [3/4] Dear ImGui (engine third_party, built once)
if not exist build\lib\imgui_te_mt.lib (
    del /q build\obj\imgui\*.obj 2>nul
    cl %CFLAGS% /DIMGUI_ENABLE_TEST_ENGINE /c %E%\third_party\imgui\imgui*.cpp %E%\third_party\imgui\backends\imgui_impl_win32.cpp %E%\third_party\imgui\backends\imgui_impl_dx12.cpp /Fo:build\obj\imgui\ || exit /b 1
    lib /nologo /OUT:build\lib\imgui_te_mt.lib build\obj\imgui\*.obj || exit /b 1
)

echo [4/4] BrickIt
rem the version in the title: VERSION, the branch and the commit (git), the platform
set "VER=0.00"
if exist VERSION set /p VER=<VERSION
set "BR=local"
set "CM=nogit"
for /f %%b in ('git rev-parse --abbrev-ref HEAD 2^>nul') do set "BR=%%b"
for /f %%c in ('git rev-parse --short HEAD 2^>nul') do set "CM=%%c"
git diff --quiet HEAD -- src assets 2>nul || set "CM=%CM%+"
> build\obj\brickit_version.h echo #define BRICKIT_VERSION L"%VER%"
>> build\obj\brickit_version.h echo #define BRICKIT_BRANCH L"%BR%"
>> build\obj\brickit_version.h echo #define BRICKIT_COMMIT L"%CM%"
>> build\obj\brickit_version.h echo #define BRICKIT_PLATFORM L"PC Win Local"
rc /nologo /fo build\obj\res\wzx.res %E%\src\app\res\wzx.rc || exit /b 1
cl %CFLAGS% /c src\brickit\*.cpp %E%\src\app\ImGuiLayer.cpp %E%\src\app\Theme.cpp %E%\src\app\Content.cpp /Fo:build\obj\brickit\ || exit /b 1
link /nologo /OUT:%OUT%\BrickIt.exe build\obj\brickit\*.obj build\obj\res\wzx.res build\lib\vp3d.lib build\lib\imgui_te_mt.lib d3d12.lib dxgi.lib d3dcompiler.lib windowscodecs.lib user32.lib gdi32.lib ole32.lib shell32.lib shcore.lib comdlg32.lib dwmapi.lib /SUBSYSTEM:WINDOWS || exit /b 1
rem Technic packs of the web builder (geometry, read as they are)
if not exist "%OUT%\assets\brickit\technic" mkdir "%OUT%\assets\brickit\technic"
xcopy /Y /Q /D docs\brickit\web\technic\g*.wasm "%OUT%\assets\brickit\technic\" >nul
xcopy /Y /Q /D docs\brickit\web\technic\index.json "%OUT%\assets\brickit\technic\" >nul
rem (the whole LDraw library is read from %LOCALAPPDATA%\BrickIt\full, docs\brickit\BIBLIOTHEQUE_COMPLETE.md)
echo Build OK: %OUT%\BrickIt.exe
