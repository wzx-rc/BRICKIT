# BrickIt on the multi-API Lit renderer of the WZX engine (Diligent Engine): the iPad build and its desktop twin.
#   iPad   : Metal through MoltenVK   (ios/, GitHub Actions, see .github/workflows/ios-build.yml)
#   Linux  : Vulkan, headless (Mesa lavapipe): BrickItLitHeadless renders a scene into a PNG, the "iPad render" of a
#            scene checked without the iPad. Use clang (Diligent built with gcc 13 loses shader resources).
#   Windows: DirectX 12 (the same image as the iPad, next to the HQ / path traced BrickIt.exe)
# The shared BrickIt code (library, building logic, stage) is compiled as it is for every target.
cmake_minimum_required(VERSION 3.21)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    project(BrickItLit CXX C OBJC OBJCXX)
else()
    project(BrickItLit CXX C)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(BRICKIT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(WZX_ENGINE_DIR "${BRICKIT_ROOT}/engine" CACHE PATH "The WZX engine (submodule engine/, branch brickit-native)")
if(NOT EXISTS "${WZX_ENGINE_DIR}/src/lit/LitRenderer.cpp")
    message(FATAL_ERROR "WZX engine not found in ${WZX_ENGINE_DIR} (git submodule update --init)")
endif()

# ---------------------------------------------------------------- Diligent Engine (core only, the player's version)
set(DILIGENT_NO_OPENGL ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_DIRECT3D11 ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_METAL ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_WEBGPU ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_ARCHIVER ON CACHE BOOL "" FORCE)
set(DILIGENT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DILIGENT_INSTALL_CORE OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(DiligentCore
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentCore.git
    GIT_TAG v2.5.6
    GIT_SHALLOW TRUE
    GIT_SUBMODULES_RECURSE TRUE)
FetchContent_MakeAvailable(DiligentCore)
FetchContent_GetProperties(DiligentCore SOURCE_DIR DILIGENT_CORE_DIR)

set(E ${WZX_ENGINE_DIR}/src)
add_library(brickit_lit STATIC
    ${E}/vp3d/Scene.cpp
    ${E}/vp3d/MeshGen.cpp
    ${E}/vp3d/EnvImage.cpp
    ${E}/vp3d/AssetImport.cpp
    ${E}/vp3d/LitPrep.cpp
    ${E}/lit/LitRenderer.cpp
    ${WZX_ENGINE_DIR}/player/src/ImGuiDiligent.cpp
    ${WZX_ENGINE_DIR}/third_party/imgui/imgui.cpp
    ${WZX_ENGINE_DIR}/third_party/imgui/imgui_draw.cpp
    ${WZX_ENGINE_DIR}/third_party/imgui/imgui_tables.cpp
    ${WZX_ENGINE_DIR}/third_party/imgui/imgui_widgets.cpp)
target_include_directories(brickit_lit PUBLIC ${E} ${BRICKIT_ROOT}/src/brickit ${CMAKE_CURRENT_SOURCE_DIR}
    ${WZX_ENGINE_DIR}/player/src ${DILIGENT_CORE_DIR} ${WZX_ENGINE_DIR}/third_party/imgui)
target_compile_definitions(brickit_lit PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE IMGUI_DEFINE_MATH_OPERATORS)
target_link_libraries(brickit_lit PUBLIC Diligent-GraphicsEngine Diligent-GraphicsTools)
if(MSVC)
    target_compile_options(brickit_lit PUBLIC /utf-8)
endif()
if(APPLE)
    target_link_libraries(brickit_lit PUBLIC "-framework ImageIO" "-framework CoreGraphics" "-framework CoreFoundation")
endif()


# ---------------------------------------------------------------- BrickIt itself: the very sources of the PC app
# (src/brickit/*.cpp, compiled unchanged; Windows-only files are empty here) on the Lit backend (GfxLit.cpp).
file(GLOB BRICKIT_APP_SOURCES ${BRICKIT_ROOT}/src/brickit/*.cpp)
add_library(brickit_app STATIC ${BRICKIT_APP_SOURCES}
    ${E}/app/Theme.cpp
    ${E}/app/Content.cpp
    GfxLit.cpp
    BrickViewer.cpp)
# compat/: <windows.h> of the shared code -> the portable subset (WinCompat.h)
target_include_directories(brickit_app PUBLIC ${BRICKIT_ROOT}/src/brickit/compat ${BRICKIT_ROOT}/third_party/stb)
target_link_libraries(brickit_app PUBLIC brickit_lit)
if(NOT MSVC)
    target_compile_options(brickit_app PRIVATE -Wno-format-security)
endif()
if(APPLE)
    target_sources(brickit_app PRIVATE ${BRICKIT_ROOT}/src/brickit/PdfApple.mm ${BRICKIT_ROOT}/src/brickit/PlatformApple.mm)
    set_source_files_properties(${BRICKIT_ROOT}/src/brickit/PdfApple.mm ${BRICKIT_ROOT}/src/brickit/PlatformApple.mm
                                PROPERTIES COMPILE_FLAGS "-fobjc-arc")
endif()

set(BRICKIT_BACKENDS)
if(D3D12_SUPPORTED)
    list(APPEND BRICKIT_BACKENDS Diligent-GraphicsEngineD3D12-static)
endif()
if(VULKAN_SUPPORTED)
    list(APPEND BRICKIT_BACKENDS Diligent-GraphicsEngineVk-static)
endif()

# Shader sources read at run time by Diligent (compiled for the backend of the device)
file(GLOB BRICKIT_SHADERS ${E}/vp3d/shaders/*.hlsl ${E}/vp3d/shaders/*.hlsli ${E}/vp3d/shaders/Shared.h
                          ${E}/lit/shaders/*.hlsl)

file(GLOB BRICKIT_TECHNIC_FILES ${BRICKIT_ROOT}/docs/brickit/web/technic/index.json ${BRICKIT_ROOT}/docs/brickit/web/technic/g*.wasm)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_executable(BrickItLitHeadless headless/main_headless.cpp)
    target_include_directories(BrickItLitHeadless PRIVATE ${DILIGENT_CORE_DIR}/ThirdParty/stb)
    target_link_libraries(BrickItLitHeadless PRIVATE brickit_lit brickit_app ${BRICKIT_BACKENDS})
    add_custom_command(TARGET BrickItLitHeadless POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:BrickItLitHeadless>/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BRICKIT_SHADERS} $<TARGET_FILE_DIR:BrickItLitHeadless>/shaders)
endif()


if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT WIN32)
    # BrickIt (the real app) without a window: renders its frames offscreen, --shot file.png writes the screen.
    add_executable(BrickItLinux linux/main_linux.cpp)
    target_link_libraries(BrickItLinux PRIVATE brickit_app ${BRICKIT_BACKENDS})
    add_custom_command(TARGET BrickItLinux POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:BrickItLinux>/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BRICKIT_SHADERS} $<TARGET_FILE_DIR:BrickItLinux>/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${WZX_ENGINE_DIR}/assets/themes $<TARGET_FILE_DIR:BrickItLinux>/assets/themes
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${BRICKIT_ROOT}/assets $<TARGET_FILE_DIR:BrickItLinux>/assets
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:BrickItLinux>/assets/brickit/technic
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BRICKIT_TECHNIC_FILES} $<TARGET_FILE_DIR:BrickItLinux>/assets/brickit/technic)
endif()

# ---------------------------------------------------------------- the iPad app (Metal through MoltenVK)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    file(READ ${BRICKIT_ROOT}/VERSION BRICKIT_VERSION)
    string(STRIP "${BRICKIT_VERSION}" BRICKIT_VERSION)
    set(BRICKIT_BUILD "1" CACHE STRING "Build number (CFBundleVersion)")
    # the assets of build\bin on the PC: BrickIt's (library, thumbnails, HUD, HDRI, fonts), the engine's themes, Technic
    file(GLOB_RECURSE BRICKIT_ASSET_FILES RELATIVE ${BRICKIT_ROOT}/assets ${BRICKIT_ROOT}/assets/*)
    set(BRICKIT_BUNDLE_FILES)
    foreach(f ${BRICKIT_ASSET_FILES})
        get_filename_component(d ${f} DIRECTORY)
        set_source_files_properties(${BRICKIT_ROOT}/assets/${f} PROPERTIES MACOSX_PACKAGE_LOCATION assets/${d})
        list(APPEND BRICKIT_BUNDLE_FILES ${BRICKIT_ROOT}/assets/${f})
    endforeach()
    file(GLOB BRICKIT_THEMES ${WZX_ENGINE_DIR}/assets/themes/*)
    set_source_files_properties(${BRICKIT_THEMES} PROPERTIES MACOSX_PACKAGE_LOCATION assets/themes)
    set_source_files_properties(${BRICKIT_TECHNIC_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION assets/brickit/technic)
    set_source_files_properties(${BRICKIT_SHADERS} PROPERTIES MACOSX_PACKAGE_LOCATION shaders)
    # the app icon (tools/brickit/app_icon.py --ios): loose PNGs at the root of the bundle, CFBundleIcons~ipad
    file(GLOB BRICKIT_ICONS ${BRICKIT_ROOT}/ios/icons/AppIcon76x76*.png ${BRICKIT_ROOT}/ios/icons/AppIcon83.5x83.5*.png)
    add_executable(BrickIt MACOSX_BUNDLE ios/main_ios.mm ${BRICKIT_SHADERS} ${BRICKIT_BUNDLE_FILES} ${BRICKIT_THEMES}
                   ${BRICKIT_TECHNIC_FILES} ${BRICKIT_ICONS})
    set_target_properties(BrickIt PROPERTIES RESOURCE "${BRICKIT_ICONS}")
    set_target_properties(BrickIt PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${BRICKIT_ROOT}/ios/Info.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER com.wzx.brickit
        MACOSX_BUNDLE_BUNDLE_NAME "BrickIt"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${BRICKIT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${BRICKIT_BUILD}"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "2"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "16.0"
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.wzx.brickit
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
    target_compile_options(BrickIt PRIVATE -fobjc-arc)
    target_link_libraries(BrickIt PRIVATE brickit_app ${BRICKIT_BACKENDS}
        "-framework UIKit" "-framework QuartzCore" "-framework Metal" "-framework IOSurface" "-framework CoreGraphics"
        "-framework UniformTypeIdentifiers" "-framework MetricKit")
endif()
