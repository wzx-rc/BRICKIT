@echo off
rem ArchViz: the building editor (src\archviz), on the BrickIt renderer (GfxDx12: HQ raster and ray tracing).
rem Builds ArchViz.exe into build\bin next to BrickIt.exe (same shaders and assets).
rem Usage: build_archviz.bat [debug]     (runs build.bat first when BrickIt has not been built yet)
setlocal
cd /d "%~dp0"
set "E=engine"

if not exist build\lib\vp3d.lib call build.bat %1 || exit /b 1
if not exist build\obj\brickit\GfxDx12.obj call build.bat %1 || exit /b 1

if defined VCToolsInstallDir goto :havevc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\archviz_vsdir.txt"
set /p VSDIR=<"%TEMP%\archviz_vsdir.txt"
if not exist "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" (
    echo Visual Studio C++ build tools not found.
    exit /b 1
)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
:havevc

set "CFG=/O2 /DNDEBUG /MT /Zi /FS"
if /i "%1"=="debug" set "CFG=/Od /Zi /MTd /D_DEBUG"
set "OUT=build\bin"
if defined VP3D_OUT set "OUT=%VP3D_OUT%"
if not exist build\obj\archviz mkdir build\obj\archviz
del /q build\obj\archviz\*.obj 2>nul

set "CFLAGS=/nologo /std:c++17 /EHsc /W3 /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /DIMGUI_DEFINE_MATH_OPERATORS %CFG% /Isrc /Isrc\brickit /Isrc\archviz /I%E%\src /I%E%\third_party\imgui /Ibuild\obj"

echo [1/2] BrickIt core (the objects of BrickIt.exe, without its entry point)
del /q build\lib\brickit_core.lib 2>nul
set "OBJS="
for %%F in (build\obj\brickit\*.obj) do if /i not "%%~nF"=="BrickAppWin" call set "OBJS=%%OBJS%% %%F"
lib /nologo /OUT:build\lib\brickit_core.lib %OBJS% || exit /b 1

echo [2/2] ArchViz
cl %CFLAGS% /c src\archviz\*.cpp /Fo:build\obj\archviz\ || exit /b 1
link /nologo /OUT:%OUT%\ArchViz.exe build\obj\archviz\*.obj build\lib\brickit_core.lib build\lib\vp3d.lib build\lib\imgui_te_mt.lib d3d12.lib dxgi.lib d3dcompiler.lib windowscodecs.lib user32.lib gdi32.lib ole32.lib shell32.lib shcore.lib comdlg32.lib dwmapi.lib dbghelp.lib winhttp.lib crypt32.lib advapi32.lib bcrypt.lib /SUBSYSTEM:WINDOWS /DEBUG /OPT:REF /OPT:ICF || exit /b 1
echo Build OK: %OUT%\ArchViz.exe
