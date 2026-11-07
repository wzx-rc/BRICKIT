// BrickIt on Linux, without a window: the real app (src/brickit, the same sources as BrickIt.exe and the iPad) on
// the Lit backend through Vulkan (Mesa lavapipe works, no GPU needed). Used to check the shared code and the iPad
// look from the cloud: same command line as BrickIt.exe (--shot file.png, --frames, --size WxH, --ui-scale ...).
//   BrickItLinux --shot out.png [--dpi 2] [BrickIt.exe options]
// The folder of the executable holds assets/ and shaders/ (copied by the build), as build\bin on the PC.
#include "BrickApp.h"
#include "GfxLit.h"
#include "Platform.h"

#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

using namespace Diligent;

int main(int argc, char** argv) {
    // the folder of the executable (build/bin), as GetModuleFileNameW on the PC
    char exe[PATH_MAX] = {};
    const ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    std::string dir = n > 0 ? std::string(exe, (size_t)n) : std::string(".");
    dir = dir.substr(0, dir.find_last_of('/'));
    if (chdir(dir.c_str()) != 0) return 1;

    float dpi = 1.0f;
    std::vector<std::wstring> args;
    for (int i = 0; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--dpi" && i + 1 < argc) { dpi = (float)std::atof(argv[++i]); continue; }   // the screen scale of the PC
        args.push_back(brickit::plat::fromUtf8(a));
    }
    brickit::Options opt;
    brickit::parseOptions(args, opt);
    if (opt.shotPath.empty()) opt.shotPath = L"brickit_linux.png";   // no window: always a capture
    const std::wstring exeDir = brickit::plat::fromUtf8(dir);
    brickit::plat::setDataDir(exeDir + L"/data");

    IEngineFactoryVk* f = GetEngineFactoryVk();
    EngineVkCreateInfo ci;
    ci.Features.ShaderResourceRuntimeArrays = DEVICE_FEATURE_STATE_OPTIONAL;
    ci.DynamicHeapSize = 64 << 20;
    ci.DynamicHeapPageSize = 2 << 20;
    RefCntAutoPtr<IRenderDevice> device;
    RefCntAutoPtr<IDeviceContext> context;
    f->CreateDeviceAndContextsVk(ci, &device, &context);
    if (!device) { std::fprintf(stderr, "No Vulkan device\n"); return 1; }
    std::printf("Device: %s\n", device->GetAdapterInfo().Description);

    const uint32_t w = (uint32_t)(opt.width * dpi), h = (uint32_t)(opt.height * dpi);
    auto renderer = std::make_unique<brickit::GfxRendererLit>();
    std::string err;
    if (!renderer->init(device, context, f, nullptr, w, h, TEX_FORMAT_RGBA8_UNORM, dir + "/shaders", &err)) {
        std::fprintf(stderr, "Lit: %s\n", err.c_str());
        return 1;
    }
    auto gui = std::make_unique<brickit::GfxGuiLit>();
    const float ui = opt.uiScale > 0 ? opt.uiScale : dpi;
    if (!gui->init(*renderer, ui, dir + "/assets/fonts", &err)) {
        std::fprintf(stderr, "ImGui: %s\n", err.c_str());
        return 1;
    }
    brickit::BrickApp app;
    if (!app.initCommon(opt, std::move(renderer), std::move(gui), exeDir, dpi)) return 0;   // (tools: done)
    int frames = 0;
    while (app.running() && frames < 100000) {
        app.frame();
        ++frames;
    }
    app.shutdownCommon();
    std::printf("%d frames\n", frames);
    return 0;
}
