// BrickIt - the graphics backend seen by the shared BrickIt code (same sources on PC and iPad).
//   PC   : GfxDx12 = WZX Engine's DirectX 12 renderer (raster HQ, path tracing, GI) + the DX12 Dear ImGui layer
//   iPad : GfxLit  = WZX Engine's Lit renderer through Diligent (Vulkan on MoltenVK) + the Diligent Dear ImGui layer
// Only the image quality differs: every UI and algorithm above this interface is common.
#pragma once
#include "imgui.h"
#include "vp3d/RenderTypes.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace brickit {

// The 3D renderer, with the subset of vp3d::Renderer that BrickIt uses (same names and meaning).
class GfxRenderer {
public:
    virtual ~GfxRenderer() = default;
    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual void shutdown() = 0;
    virtual void waitIdle() = 0;
    virtual bool ready() const = 0;   // the device exists (window messages may come before the init)
    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;

    virtual void setExtraSelection(const std::vector<int>& objects) = 0;
    virtual void setEdgeOutline(const std::vector<int>& objects, bool red, float opacity) = 0;
    // Renders the scene into the viewport rectangle of the back buffer, or offscreen (toBackBuffer = false, the call
    // waits: thumbnails), the image then readable by readViewport.
    virtual void render(const vp3d::Scene& scene, const vp3d::RenderSettings& rs, const vp3d::ViewRect& vp,
                        int selectedObject, bool toBackBuffer = true) = 0;
    // Same, the image kept for the UI (GfxGui::viewportTexture).
    virtual void renderToTexture(const vp3d::Scene& scene, const vp3d::RenderSettings& rs, const vp3d::ViewRect& vp,
                                 int selectedObject) = 0;
    virtual bool readViewport(std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h) = 0;
    virtual bool readBackBuffer(std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h) = 0;
    virtual void present(bool vsync) = 0;

    virtual void resetAccumulation() = 0;
    virtual uint32_t sampleCount() const = 0;
    virtual float gpuFrameMs() const = 0;
    virtual uint64_t rasterTriangles() const = 0;
    virtual float giUpdateMs() const = 0;
    virtual float giBlendMs() const = 0;
    virtual bool pathTracing() const = 0;   // false on the iPad: raster only, the PT settings are ignored
};

// Dear ImGui on top of the renderer: platform frame, draw, textures for thumbnails.
class GfxGui {
public:
    virtual ~GfxGui() = default;
    virtual void shutdown() = 0;
    virtual void newFrame() = 0;
    virtual void render(bool viewportTextureInUse) = 0;   // the UI over the back buffer
    virtual void onResize() = 0;
    virtual ImTextureID viewportTexture() = 0;
    virtual ImTextureID createTexture(const uint8_t* rgba, uint32_t w, uint32_t h) = 0;   // 0 on failure
    virtual void releaseTexture(ImTextureID id) = 0;

    ImFont* fontBold = nullptr;
    ImFont* fontSmall = nullptr;
    float dpi = 1.0f;
    std::function<void()> beforeNewFrame;   // self-test: scripted input between the platform backend and NewFrame
};

} // namespace brickit
