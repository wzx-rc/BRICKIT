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
    ${BRICKIT_ROOT}/src/brickit/Platform.cpp
    ${BRICKIT_ROOT}/src/brickit/Library.cpp
    ${BRICKIT_ROOT}/src/brickit/Build.cpp
    ${BRICKIT_ROOT}/src/brickit/Deflate.cpp
    ${BRICKIT_ROOT}/src/brickit/MeshLod.cpp
    ${BRICKIT_ROOT}/src/brickit/Stage.cpp
    ${WZX_ENGINE_DIR}/player/src/ImGuiDiligent.cpp
    ${WZX_ENGINE_DIR}/third_party/imgui/imgui.cpp
    ${WZX_ENGINE_DIR}/third_party/imgui/imgui_draw.cpp
    ${WZX_ENGINE_DIR}/third_party/imgui/imgui_tables.cpp
    ${WZX_ENGINE_DIR}/third_party/imgui/imgui_widgets.cpp
    BrickViewer.cpp)
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

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_executable(BrickItLitHeadless headless/main_headless.cpp)
    target_include_directories(BrickItLitHeadless PRIVATE ${DILIGENT_CORE_DIR}/ThirdParty/stb)
    target_link_libraries(BrickItLitHeadless PRIVATE brickit_lit ${BRICKIT_BACKENDS})
    add_custom_command(TARGET BrickItLitHeadless POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:BrickItLitHeadless>/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BRICKIT_SHADERS} $<TARGET_FILE_DIR:BrickItLitHeadless>/shaders)
endif()

# ---------------------------------------------------------------- the iPad app (Metal through MoltenVK)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    file(READ ${BRICKIT_ROOT}/VERSION BRICKIT_VERSION)
    string(STRIP "${BRICKIT_VERSION}" BRICKIT_VERSION)
    set(BRICKIT_BUILD "1" CACHE STRING "Build number (CFBundleVersion)")
    set(BRICKIT_ASSETS
        ${BRICKIT_ROOT}/assets/brickit/base.json
        ${BRICKIT_ROOT}/assets/brickit/base.bin
        ${BRICKIT_ROOT}/assets/brickit/part_colors.txt
        ${BRICKIT_ROOT}/assets/brickit/default_scene.v3d.json
        ${BRICKIT_ROOT}/assets/brickit/default_prefs.json)
    file(GLOB BRICKIT_TECHNIC ${BRICKIT_ROOT}/docs/brickit/web/technic/index.json ${BRICKIT_ROOT}/docs/brickit/web/technic/g*.wasm)
    add_executable(BrickIt MACOSX_BUNDLE ios/main_ios.mm ${BRICKIT_SHADERS} ${BRICKIT_ASSETS} ${BRICKIT_TECHNIC})
    set_source_files_properties(${BRICKIT_SHADERS} PROPERTIES MACOSX_PACKAGE_LOCATION shaders)
    set_source_files_properties(${BRICKIT_ASSETS} PROPERTIES MACOSX_PACKAGE_LOCATION assets/brickit)
    set_source_files_properties(${BRICKIT_TECHNIC} PROPERTIES MACOSX_PACKAGE_LOCATION assets/brickit/technic)
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
    target_link_libraries(BrickIt PRIVATE brickit_lit ${BRICKIT_BACKENDS}
        "-framework UIKit" "-framework QuartzCore" "-framework Metal" "-framework IOSurface" "-framework CoreGraphics")
endif()
