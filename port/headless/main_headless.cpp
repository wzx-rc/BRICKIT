// BrickIt "iPad render" without the iPad: the BrickViewer of the iPad app (shared BrickIt code + Lit renderer + HUD)
// renders a scene into a PNG and prints the timings.
//   BrickItLitHeadless --assets <BRICKIT>/assets/brickit [--scene file.v3d.json | --bench N] [--out image.png]
//                      [--width 2388 --height 1668] [--frames 5] [--hdri sky|none|file.hdr] [--ddgi 1]
//                      [--ui-scale 2] [--no-hud] [--shaders dir1;dir2] [--backend vk|d3d12]
// Linux: Vulkan on Mesa lavapipe (no GPU). Windows: DirectX 12 by default.
#include "BrickViewer.h"
#include "imgui.h"

#include "Graphics/GraphicsEngine/interface/EngineFactory.h"
#if D3D12_SUPPORTED
#include "Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h"
#endif
#if VULKAN_SUPPORTED
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#endif
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace Diligent;

static double nowMs() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

int main(int argc, char** argv) {
    std::string out = "brickit_lit.png", backend, shaders = "shaders";
    uint32_t width = 2388, height = 1668;   // iPad Pro 11" (2nd generation), landscape
    int frames = 5;
    brickit::ViewerOptions opt;
    opt.assets = "assets/brickit";
#if D3D12_SUPPORTED
    backend = "d3d12";
#else
    backend = "vk";
#endif
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if (a == "--out") out = next();
        else if (a == "--width") width = (uint32_t)std::atoi(next().c_str());
        else if (a == "--height") height = (uint32_t)std::atoi(next().c_str());
        else if (a == "--frames") frames = std::atoi(next().c_str());
        else if (a == "--backend") backend = next();
        else if (a == "--shaders") shaders = next();
        else if (a == "--assets") opt.assets = next();
        else if (a == "--data") opt.dataDir = next();
        else if (a == "--scene") opt.scene = next();
        else if (a == "--bench") opt.bench = std::atoi(next().c_str());
        else if (a == "--hdri") opt.hdri = next();
        else if (a == "--hdri-rot") opt.hdriRotation = (float)std::atof(next().c_str());
        else if (a == "--hdri-int") opt.hdriIntensity = (float)std::atof(next().c_str());
        else if (a == "--ui-scale") opt.uiScale = (float)std::atof(next().c_str());
        else if (a == "--touch") opt.touch = true;
        else if (a == "--no-hud") opt.hud = false;
        else if (a == "--ddgi") opt.ddgi = std::atoi(next().c_str()) != 0;
    }

    RefCntAutoPtr<IRenderDevice> device;
    RefCntAutoPtr<IDeviceContext> context;
    IEngineFactory* factory = nullptr;
#if D3D12_SUPPORTED
    if (backend == "d3d12") {
        IEngineFactoryD3D12* f = GetEngineFactoryD3D12();
        if (f && f->LoadD3D12()) {
            EngineD3D12CreateInfo ci;
            ci.Features.ShaderResourceRuntimeArrays = DEVICE_FEATURE_STATE_OPTIONAL;
            ci.DynamicHeapPageSize = 8 << 20;
            f->CreateDeviceAndContextsD3D12(ci, &device, &context);
            factory = f;
        }
    }
#endif
#if VULKAN_SUPPORTED
    if (backend == "vk") {
        IEngineFactoryVk* f = GetEngineFactoryVk();
        EngineVkCreateInfo ci;
        ci.Features.ShaderResourceRuntimeArrays = DEVICE_FEATURE_STATE_OPTIONAL;
        ci.DynamicHeapSize = 32 << 20;
        ci.DynamicHeapPageSize = 2 << 20;
        f->CreateDeviceAndContextsVk(ci, &device, &context);
        factory = f;
    }
#endif
    if (!device) {
        std::fprintf(stderr, "No %s device\n", backend.c_str());
        return 1;
    }
    std::printf("Device: %s (%s)\n", device->GetAdapterInfo().Description, backend.c_str());

    double t0 = nowMs();
    brickit::BrickViewer viewer;
    std::string err;
    if (!viewer.init(device, context, factory, TEX_FORMAT_RGBA8_UNORM, shaders, opt, &err)) {
        std::fprintf(stderr, "%s\n", err.c_str());
        return 1;
    }
    std::printf("%s\nInit: %.0f ms\n", viewer.log().c_str(), nowMs() - t0);

    TextureDesc td;
    td.Name = "BrickIt output";
    td.Type = RESOURCE_DIM_TEX_2D;
    td.Width = width;
    td.Height = height;
    td.Format = TEX_FORMAT_RGBA8_UNORM;
    td.BindFlags = BIND_RENDER_TARGET;
    RefCntAutoPtr<ITexture> target, staging;
    device->CreateTexture(td, nullptr, &target);
    td.Name = "BrickIt readback";
    td.BindFlags = BIND_NONE;
    td.Usage = USAGE_STAGING;
    td.CPUAccessFlags = CPU_ACCESS_READ;
    device->CreateTexture(td, nullptr, &staging);
    if (!target || !staging) return 1;

    for (int f = 0; f < std::max(frames, 1); ++f) {
        const double a = nowMs();
        viewer.frame(target->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET), width, height, 1.0f / 60.0f, brickit::TouchCamera());
        const double b = nowMs();
        context->Flush();
        context->FinishFrame();
        context->WaitForIdle();
        std::printf("Frame %d: CPU %.1f ms, CPU+GPU %.1f ms, %u draw calls\n", f, b - a, nowMs() - a, viewer.renderer().drawCalls());
    }
    CopyTextureAttribs copy(target, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, staging, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    context->CopyTexture(copy);
    context->WaitForIdle();
    MappedTextureSubresource m;
    context->MapTextureSubresource(staging, 0, 0, MAP_READ, MAP_FLAG_DO_NOT_WAIT, nullptr, m);
    std::vector<uint8_t> rgba((size_t)width * height * 4);
    for (uint32_t y = 0; y < height; ++y)
        std::memcpy(&rgba[(size_t)y * width * 4], (const uint8_t*)m.pData + (size_t)y * m.Stride, (size_t)width * 4);
    context->UnmapTextureSubresource(staging, 0, 0);
    if (!stbi_write_png(out.c_str(), (int)width, (int)height, 4, rgba.data(), (int)width * 4)) return 1;
    std::printf("Wrote %s (%ux%u, MSAA %ux)\n", out.c_str(), width, height, viewer.renderer().msaaSamples());
    viewer.shutdown();
    return 0;
}
