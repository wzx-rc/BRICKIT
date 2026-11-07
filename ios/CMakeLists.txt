# BrickIt iPad (Metal). Built on a GitHub macOS runner (.github/workflows/ios-build.yml), no Mac needed:
#   cmake -S ios -B build/ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64
# The .app is unsigned; Sideloadly signs it on the PC with a free Apple ID and installs it on the iPad.
# J0 (docs/brickit/PORTAGE_IPAD.md): an empty Metal app that validates the build and install chain.
cmake_minimum_required(VERSION 3.24)
project(BrickItIPad LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../VERSION BRICKIT_VERSION)
string(STRIP "${BRICKIT_VERSION}" BRICKIT_VERSION)
set(BRICKIT_GIT "local" CACHE STRING "Short git hash shown in the app")
set(BRICKIT_BUILD "1" CACHE STRING "Build number (CFBundleVersion)")

add_executable(BrickIt MACOSX_BUNDLE main_ios.mm)
target_compile_definitions(BrickIt PRIVATE BRICKIT_VERSION="${BRICKIT_VERSION}" BRICKIT_GIT="${BRICKIT_GIT}")
set_target_properties(BrickIt PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER com.wzx.brickit
    MACOSX_BUNDLE_BUNDLE_NAME "BrickIt"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${BRICKIT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${BRICKIT_BUILD}"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "2"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "16.0"
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.wzx.brickit
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
target_link_libraries(BrickIt PRIVATE
    "-framework UIKit" "-framework QuartzCore" "-framework Metal" "-framework CoreGraphics")
